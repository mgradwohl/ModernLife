#include "pch.h"
#include "Renderer.h"

#include <functional>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "Log.h"
#include "Board.h"
#include "HSVColorHelper.h"

using namespace winrt;

void Renderer::Attach(const Microsoft::Graphics::Canvas::CanvasDevice& device, float dpi, uint16_t maxindex)
{
    ML_METHOD;

    // these can change externally
    _canvasDevice = device;
    _dpi = dpi;
    _spriteMaxIndex = maxindex;

    // determined internally
    _threadcount = gsl::narrow_cast<int>(std::thread::hardware_concurrency() / 2);
    _threadcount = std::clamp(_threadcount, 2, 8);
    //_threadcount = 1;
}

void Renderer::SetupRenderTargets()
{
    ML_METHOD;

    // Calculate important internal vales for the spritesheet and backbuffer slices
    _dipsPerCellDimension = gsl::narrow_cast<float>(_bestbackbuffersize / _boardwidth);

    _spritesPerRow = gsl::narrow_cast<uint16_t>(1 + std::sqrt(_spriteMaxIndex));
    _spriteDipsPerRow = gsl::narrow_cast<float>(_dipsPerCellDimension * _spritesPerRow);
    _rowsPerSlice = gsl::narrow_cast<uint16_t>(_boardheight / _threadcount);
    _sliceHeight = gsl::narrow_cast<float>(_rowsPerSlice * _dipsPerCellDimension);

    if (_threadcount == 1)
    {
        {
            std::scoped_lock lock{ _lockbackbuffer };
            _backbuffersingle = Microsoft::Graphics::Canvas::CanvasRenderTarget(_canvasDevice, _bestbackbuffersize, _bestbackbuffersize, _dpi);
        }
        BuildSpriteSheet();
        return;
    }

    {
        std::scoped_lock lock{ _lockbackbuffer };
        // create backbuffers that are sliced horizontally they will be as evenly divided as possible
        // with the final slice potentially being larger
        _backbuffers.clear();
        int j = 0;
        for (j = 0; j < _threadcount - 1; j++)
        {
            _backbuffers.emplace_back(Microsoft::Graphics::Canvas::CanvasRenderTarget{ _canvasDevice, _bestbackbuffersize, _sliceHeight, _dpi });
        }
        const int remainingRows = _boardheight - (j * _rowsPerSlice);
        _backbuffers.emplace_back(Microsoft::Graphics::Canvas::CanvasRenderTarget{ _canvasDevice, _bestbackbuffersize, gsl::narrow_cast<float>(remainingRows * _dipsPerCellDimension), _dpi });
    }

    BuildSpriteSheet();
}

[[nodiscard]] GridPoint Renderer::GetCellAtPoint(Windows::Foundation::Point point) noexcept
{
    _imagescale = _bestbackbuffersize / _bestcanvassize;

    return { gsl::narrow_cast<uint16_t>(point.X / _dipsPerCellDimension * _imagescale), gsl::narrow_cast<uint16_t>(point.Y / _dipsPerCellDimension * _imagescale) };
}

// How Render works
// 1.   Calls RenderOffScreen, the only method that needs the board
//      Which splits the board into horizontal slices, and creates a CanvasDrawingSession for each backbuffer slice
//      that was created by SetupRenderTargets. It then creates a thread per slice and each thread is constructed with
//      DrawHorizontalRows to draw the correct rows into the correct slice
// 2.   DrawHorizonalRows then renders its slice into the corresponding backbuffer 
//      by drawing the correct sprite from the spritesheet
// 3.   When all the threads created by RenderOffScreen join (which they do automatically) control
//      is returned to Render, which then draws each backbuffer slice into the front buffer
 
void Renderer::Render(const Microsoft::Graphics::Canvas::CanvasDrawingSession& ds, const Board& board)
{
    RenderOffscreen(board);

    if (_threadcount == 1)
    {
        {
            std::scoped_lock lock{ _lockbackbuffer };

            const Windows::Foundation::Rect source{ 0.0f, 0.0f, _bestbackbuffersize, _bestbackbuffersize };
            const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _bestcanvassize, _bestcanvassize };
            ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Aliased);
            ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);
            ds.DrawImage(_backbuffersingle, destRect, source);

            ds.Flush();
            ds.Close();
        }
        return;
    }

    const float canvasSliceHeight = _sliceHeight / (_bestbackbuffersize / _bestcanvassize);
    Windows::Foundation::Rect source{ 0.0f, 0.0f, _bestbackbuffersize, _sliceHeight };
    Windows::Foundation::Rect dest{ 0.0f, 0.0f, _bestcanvassize,  canvasSliceHeight };
    const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _bestcanvassize, _bestcanvassize };

    ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Aliased);
    ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);

    // lock the full size backbuffer and copy each slice into it
    std::scoped_lock lock{ _lockbackbuffer };

    int k = 0;
    for (k = 0; k < _threadcount - 1; k++)
    {
        ds.DrawImage(gsl::at(_backbuffers, k), dest, source);
        dest.Y += canvasSliceHeight;
    }
    dest.Height = gsl::narrow_cast<float>(_bestcanvassize - (canvasSliceHeight * k));
    source.Height = gsl::narrow_cast<float>(_bestbackbuffersize - (_sliceHeight * k));
    ds.DrawImage(gsl::at(_backbuffers, k), dest, source);

    ds.Flush();
    ds.Close();
}

// This renders the board to the backbuffers
void Renderer::RenderOffscreen(const Board& board)
{
    //https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

    if (_threadcount == 1)
    {
        Microsoft::Graphics::Canvas::CanvasDrawingSession dsSingle = _backbuffersingle.CreateDrawingSession();
        DrawHorizontalRows(dsSingle, board, 0, _boardheight);
        return;
    }

    // create a drawing session for each backbuffer horizontal slice
    _dsList.clear();
    for (int j = 0; j < _threadcount; j++)
    {
        _dsList.push_back({ gsl::at(_backbuffers, j).CreateDrawingSession() });
    }

    // technically the board could be changing underneath us, but we're only reading the cells not writing to them
    // TODO may need to lock the board here eg std::scoped_lock lock{ _board.GetLock() };
    uint16_t startRow = 0;
    std::vector<std::jthread> threads;
    int t = 0;
    for (t = 0; t < _threadcount - 1; t++)
    {
        threads.emplace_back(std::jthread{ &Renderer::DrawHorizontalRows, this, gsl::at(_dsList, t), std::ref(board), startRow, gsl::narrow_cast<uint16_t>(startRow + _rowsPerSlice) });
        startRow += _rowsPerSlice;
    }
    threads.emplace_back(std::jthread{ &Renderer::DrawHorizontalRows, this, gsl::at(_dsList, t), std::ref(board), startRow, board.Height() });
}

void Renderer::DrawHorizontalRows(const Microsoft::Graphics::Canvas::CanvasDrawingSession& ds, const Board& board, uint16_t startRow, uint16_t endRow) const
{
    // only read from the board/the cells in this method
    ds.Clear(Windows::UI::Colors::WhiteSmoke());
#ifdef _DEBUG
    // this makes the background of each backbuffer slice a band so you can see them
    ds.Clear(Windows::UI::ColorHelper::FromArgb(255, 255 - gsl::narrow_cast<uint8_t>(startRow), 0, gsl::narrow_cast<uint8_t>(startRow)));
#endif // DEBUG

    ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Antialiased);
    ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);
    Microsoft::Graphics::Canvas::CanvasSpriteBatch spriteBatch = ds.CreateSpriteBatch(Microsoft::Graphics::Canvas::CanvasSpriteSortMode::Bitmap, Microsoft::Graphics::Canvas::CanvasImageInterpolation::NearestNeighbor, Microsoft::Graphics::Canvas::CanvasSpriteOptions::None);

    Windows::Foundation::Rect rectDest{ 0.0f, 0.0f, _dipsPerCellDimension, _dipsPerCellDimension };
    {
        for (uint16_t y = startRow; y < endRow; y++)
        {
            for (uint16_t x = 0; x < board.Width(); x++)
            {
                const Cell& cell = board.GetCell(x, y);

                rectDest.X = gsl::narrow_cast<float>(x * _dipsPerCellDimension);
                rectDest.Y = gsl::narrow_cast<float>((y - startRow) * _dipsPerCellDimension);
                if (cell.ShouldDraw())
                {
                    // this is where all the time goes:
                    spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, GetSpriteCell(cell.Age()));
                }
            }
        }
    }
    spriteBatch.Close();
    ds.Flush();
    ds.Close();
}

[[nodiscard]] Windows::Foundation::Rect Renderer::GetSpriteCell(uint16_t index) const noexcept
{
    const uint16_t i = std::clamp(index, gsl::narrow_cast<uint16_t>(0), gsl::narrow_cast<uint16_t>(_spriteMaxIndex + 1));
    const Windows::Foundation::Rect rect{ (i % _spritesPerRow) * _dipsPerCellDimension, (i / _spritesPerRow) * _dipsPerCellDimension, _dipsPerCellDimension, _dipsPerCellDimension };

    return rect;
}

void Renderer::BuildSpriteSheet()
{
    ML_METHOD;

    // TODO only fill most of the cells with color. Reserve maybe the last 10% or so for gray
    // TODO gray is h=0, s=0, and v from 1 to 0
    // this will be used to iterate through the width and height of the rendertarget *without* adding a partial tile at the end of a row

    // Since we're using pixels, but the _dipsPerCellAxis is in dips there is already "whitespace padding"
    // in the sprite so we'll take advantage of that
    // TODO maybe this is unneccessary
    float offset = 1.0f;
    if (_dipsPerCellDimension < 14.0f)
    {
        offset = 0.5f;
    }

    if (_dipsPerCellDimension < 7.0f)
    {
        offset = 0.25f;
    }

    constexpr float round = 4.0f;
    const float inset = _dipsPerCellDimension / 4.0f;
    uint16_t index = 0;
    float posx{ 0.0f };
    float posy{ 0.0f };

    // create a square render target that will hold all the tiles (this will avoid a partial 'tile' at the end which we won't use)
    {
        std::scoped_lock lock{ _lockbackbuffer };

        _spritesheet = Microsoft::Graphics::Canvas::CanvasRenderTarget(_canvasDevice, _spriteDipsPerRow, _spriteDipsPerRow, _dpi);

        Microsoft::Graphics::Canvas::CanvasDrawingSession ds = _spritesheet.CreateDrawingSession();
        ds.Clear(Windows::UI::Colors::WhiteSmoke());
        ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Antialiased);
        ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);


        for (uint16_t y = 0; y < _spritesPerRow; y++)
        {
            for (uint16_t x = 0; x < _spritesPerRow; x++)
            {
                ds.FillRoundedRectangle(posx + offset, posy + offset, _dipsPerCellDimension - (2 * offset), _dipsPerCellDimension - (2 * offset), round, round, GetOutlineColorHSV(index));
                ds.FillRoundedRectangle(posx + inset, posy + inset, _dipsPerCellDimension - (2 * inset), _dipsPerCellDimension - (2 * inset), round, round, GetCellColorHSV(index));

                posx += _dipsPerCellDimension;
                index++;
            }
            posy += _dipsPerCellDimension;
            posx = 0.0f;
        }

        ds.Flush();
        ds.Close();
    }
}

void Renderer::Size(uint16_t width, uint16_t height)
{
    if (_boardwidth != width || _boardheight != height)
    {
        _boardwidth = width;
        _boardheight = height;
        SetupRenderTargets();
	}
}

void Renderer::Device(const Microsoft::Graphics::Canvas::CanvasDevice& device)
{
    if (_canvasDevice != device)
    {
		_canvasDevice = device;
		SetupRenderTargets();
	}
}

void Renderer::Dpi(float dpi)
{
    if (_dpi != dpi)
    {
        // TODO if the dpi changes we need to rebuild the spritesheet and much more
        _dpi = dpi;
        BuildSpriteSheet();
    }
}

void Renderer::SpriteMaxIndex(uint16_t index)
{
    if (_spriteMaxIndex != index)
    {
		_spriteMaxIndex = index;
        SetupRenderTargets();
	}
}

void Renderer::FindBestCanvasSize(size_t windowHeight)
{
    ML_METHOD;

    // determine the right size for the canvas
    // lock these because they could change underneath a draw
    {
        std::scoped_lock lock{ _lockbackbuffer };

        float best = 400.0f;
        while (true)
        {
            if (gsl::narrow_cast<size_t>((best * _dpi / 96.0f)) >= windowHeight) break;
            best += 100.0f;
        }
        best -= 200.0f;
        _bestcanvassize = best;

        // make the backbuffer bigger than the front buffer, and a multiple of it
        _bestbackbuffersize = _bestcanvassize;
        while (_bestbackbuffersize < _idealbackbuffersize)
        {
            _bestbackbuffersize += _bestcanvassize;
        }
    }
}

// color helpers used by spritesheet
[[nodiscard]] Windows::UI::Color Renderer::GetOutlineColorHSV(uint16_t age) const
{
    if (age >= _spriteMaxIndex)
    {
        return Windows::UI::Colors::DarkGray();
    }

    const float h{ gsl::narrow_cast<float>((age * 360.f) / _spriteMaxIndex) };
    return HSVColorHelper::HSVtoColor(h, 0.6f, 0.7f);
}

[[nodiscard]] Windows::UI::Color Renderer::GetCellColorHSV(uint16_t age) const
{
    if (age >= _spriteMaxIndex)
    {
        return Windows::UI::Colors::Gray();
    }

    const float h{ gsl::narrow_cast<float>((age * 360.f) / _spriteMaxIndex) };
    return HSVColorHelper::HSVtoColor(h, 0.7f, 0.9f);
}