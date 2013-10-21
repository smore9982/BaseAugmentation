/*==============================================================================
Copyright (c) 2012-2013 QUALCOMM Austria Research Center GmbH.
All Rights Reserved.

@file
    Transition3Dto2D.h

@brief
    A utility class for animating a textured 3D plane from target space
    to 2D screen space.

==============================================================================*/

#ifndef _QCAR_TRANSITION_3D_TO_2D_H_
#define _QCAR_TRANSITION_3D_TO_2D_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <QCAR/QCAR.h>
#include <QCAR/Renderer.h>
#include <QCAR/Image.h>

class Transition3Dto2D
{
public:

    Transition3Dto2D(int screenWidth, int screenHeight, bool isPortraitMode,
            float dpiSIndicator, float scaleFactor);
    ~Transition3Dto2D();

    // Call this from the GL thread
    void initializeGL(unsigned int sProgramID);

    // Center of the screen is (0, 0)
    // centerX and centerY are pixel offsets from this point
    // width and height are also in pixels
    void setScreenRect(int centerX, int centerY, int width, int height);

    // Call this once to set up the transition
    // Note: inReverse and keepRendering are not currently used
    void startTransition(float duration, bool inReverse, bool keepRendering);

    // Transitions between textures 1 and 2
    // Transitions between target space and screen space
    void render(QCAR::Matrix44F projectionMatrix, QCAR::Matrix34F targetPose,
            GLuint texture1);

    // Returns true if transition has finished animating
    bool transitionFinished();

    void updateScreenPoperties(int screenWidth, int screenHeight,
            bool isPortraitMode);

private:

    bool isActivityPortraitMode;
    int screenWidth;
    int screenHeight;
    QCAR::Vec4F screenRect;
    QCAR::Matrix44F identityMatrix;
    QCAR::Matrix44F orthoMatrix;

    unsigned int shaderProgramID;
    GLint normalHandle;
    GLint vertexHandle;
    GLint textureCoordHandle;
    GLint mvpMatrixHandle;

    float animationLength;
    int animationDirection;
    bool renderAfterCompletion;
    float dpiScaleIndicator;
    unsigned long animationStartTime;
    bool animationFinished;
    float scaleFactor;

    float stepTransition();
    QCAR::Matrix44F getFinalPositionMatrix();
    float deccelerate(float val);
    float accelerate(float val);
    void linearInterpolate(QCAR::Matrix44F* start, QCAR::Matrix44F* end,
            QCAR::Matrix44F* current, float elapsed);

    unsigned long getCurrentTimeMS();

};

#endif //_QCAR_TRANSITION_3D_TO_2D_H_
