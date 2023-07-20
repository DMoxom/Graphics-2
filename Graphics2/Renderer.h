#pragma once

class Renderer
{
public:
	Renderer() {}
	virtual ~Renderer() {}

	virtual bool Initialise() = 0;
	virtual void Render() = 0;
	virtual void Shutdown() {};
};

