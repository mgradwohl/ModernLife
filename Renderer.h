#pragma once

#include <mutex>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "Board.h"

using namespace winrt;

class Renderer
{
public:
	// construct
	Renderer() = default;
	void Attach(const Microsoft::Graphics::Canvas::CanvasDevice& device, float dpi, uint16_t maxindex);

	// assign
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	// destruct
	~Renderer() = default;

	// setters for render property changes
	void Dpi(float dpi);
	void SpriteMaxIndex(uint16_t index);
	void Size(uint16_t width, uint16_t height);
	void Device(const Microsoft::Graphics::Canvas::CanvasDevice& device);

	void FindBestCanvasSize(size_t windowHeight);
	void WindowResize();

	GridPoint GetCellAtPoint(Windows::Foundation::Point point) noexcept;

	void Render(Microsoft::Graphics::Canvas::CanvasDrawingSession const& ds, const Board& board);

	[[nodiscard]] float DipsPerCell() noexcept
	{
		return _dipsPerCellDimension;
	}

	[[nodiscard]] int ThreadCount() noexcept
	{
		return _threadcount;
	}

	[[nodiscard]] float CanvasSize() noexcept
	{
		return _bestcanvassize;
	}

	[[nodiscard]] float CanvasHeight() noexcept
	{
		return _bestcanvassize;
	}

	[[nodiscard]] float CanvasWidth() noexcept
	{
		return _bestcanvassize;
	}

	[[nodiscard]] float BackbufferSize() noexcept		
	{
		return _bestbackbuffersize;
	}

	[[nodiscard]] Microsoft::Graphics::Canvas::CanvasRenderTarget& SpriteSheet() noexcept
	{
		return _spritesheet;
	}

private:
	Windows::Foundation::Rect GetSpriteCell(uint16_t index) const noexcept;
	void SetupRenderTargets();
	void BuildSpriteSheet();
	void DrawHorizontalRows(const Microsoft::Graphics::Canvas::CanvasDrawingSession& ds, const Board& board, uint16_t startRow, uint16_t endRow) const;
	void RenderOffscreen(const Board& board);
	Windows::UI::Color GetCellColorHSV(uint16_t age) const;
	Windows::UI::Color GetOutlineColorHSV(uint16_t age) const;

private:
	std::mutex _lockbackbuffer;

	Microsoft::Graphics::Canvas::CanvasRenderTarget _backbuffersingle{nullptr};

	std::vector<Microsoft::Graphics::Canvas::CanvasRenderTarget> _backbuffers;
	std::vector<Microsoft::Graphics::Canvas::CanvasDrawingSession> _dsList;
	Microsoft::Graphics::Canvas::CanvasRenderTarget _spritesheet{ nullptr };
	Microsoft::Graphics::Canvas::CanvasDevice _canvasDevice{ nullptr };

	int _threadcount{ 0 };
	unsigned int _pxPerCellDimension{ 0 };
	float _dpi{ 0.0f };
	float _imagescale{ 0.0f };
	float _dipsPerCellDimension{ 0.0f };
	float _sliceHeight{ 0.0f };
	float _spriteDipsPerRow{ 0.0f };
	uint16_t _rowsPerSlice{ 0 };
	uint16_t _spritesPerRow{ 0 };
	uint16_t _boardwidth{ 0 };
	uint16_t _boardheight{ 0 };
	uint16_t _spriteMaxIndex{ 0 };
	float _bestcanvassize{ 1000.0f };
	float _idealbackbuffersize{ 2000.0f };
	float _bestbackbuffersize{ 2000.0f };
};