﻿#ifndef ELEMENT_BASE_H
#define ELEMENT_BASE_H

#define GL_VIEW_PORT_NORM_WIDTH     2.0f
#define GL_VIEW_PORT_NORM_HEIGHT    2.0f

// ----------------------------- <- GL view port top
// |                           |
// |                           |
// |---------------------------| <- screen top
// |                           |
// |                           |
// |                           |
// |                           |
// |                           |
// |---------------------------| <- screen bottom
// |                           |
// |                           |
// ----------------------------- <- GL view port bottom
// ^ screen left               ^ screen right
//   GL view port left           GL view port right

namespace fezrestia {

class ElementBase {

public:
    // CONSTRUCTOR.
    ElementBase();

    // DESTRUCTOR.
    virtual ~ElementBase();

    /**
     * Initialize.
     */
    void initialize(float screenNormWidth, float screenNormHeight);

    /**
     * Finalize.
     */
    void finalize();

    /**
     * Update global position / posture matrix
     */
    void setGlobalMatrix(float* matrix);

    /**
     * Set visibility.
     */
    void setVisibility(GLboolean isVisible);

    /**
     * This element is visible or not.
     */
    GLboolean isVisible();

    /**
     * Request rendering.
     */
    virtual void render() = 0;

    /**
     * Translate this element and all children.
     * Call this after setGlobalMatrix().
     */
    void translate(float transX, float transY, float transZ);

    /**
     * Rotate this element and all children.
     * Call this after setGlobalMatrix().
     */
    void rotate(float rotDeg, float axisX, float axisY, float axisZ);

    /**
     * Scale this element and all children.
     * Call this after setGlobalMatrix().
     */
    void scale(float scaleX, float scaleY, float scaleZ);

protected:
    /**
     * Normalized screen width.
     */
    float getScreenNormWidth();

    /**
     * Normalized screen height.
     */
    float getScreenNormHeight();

    /**
     * Global matrix.
     */
    float* getGlobalMatrix();

    /**
     * Local matrix.
     */
    float* getSequencedLocalMatrix();

private:
    //// MEMBER FIELDS ///////////////////////////////////////////////////////////////////////////

    /** Screen normalized size. */
    float mScreenNormWidth;
    float mScreenNormHeight;

    /** Global matrix. */
    float mGlobalMatrix[16];

    /** Local matrix. This will be multiplied to global matrix before render. */
    float mSequencedLocalMatrix[16];

    /** Visibility flag. */
    GLboolean mIsVisible;

    //// FUNCTIONS ///////////////////////////////////////////////////////////////////////////////

};

}; // namespace fezrestia

#endif // ELEMENT_BASE_H
