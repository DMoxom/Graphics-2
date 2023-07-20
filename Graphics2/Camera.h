#pragma once
#include "core.h"
#include "DirectXCore.h"

class Camera
{
public:
    Camera();
    ~Camera();

    void Update();
    XMMATRIX GetViewMatrix();
    XMVECTOR GetCameraPosition();
    void SetCameraPosition(float x, float y, float z);
    void SetPitch(float pitch);
	void SetTotalPitch(float pitch);
	float GetPitch() const;
    void SetYaw(float yaw);
	void SetTotalYaw(float yaw);
	float GetYaw() const;
	void SetRoll(float roll);
	void SetTotalRoll(float roll);
	float GetRoll() const;
	void SetLeftRight(float leftRight);
    void SetForwardBack(float forwardBack);

private:
    XMFLOAT4    _cameraPosition;

    XMFLOAT4X4  _viewMatrix;

    float       _moveLeftRight;
    float       _moveForwardBack;

    float       _cameraYaw;
    float       _cameraPitch;
    float       _cameraRoll;

};

