#ifndef TRANSFORM_H
#define TRANSFORM_H

/** multiplies two transforms together witht the result in t1.
 * \param t1 in/output array of 16 floats
 * \param t2 in only array of 16 floats.
 */
void multTransform(float *t1,float *t2);
/** rotates a transform by a rotation angle in X, Y and Z planes.
 * \param transform in/out result
 * \param xrot rotation around x axis (in radians)
 * \param yrot rotation around y axis (in radians)
 * \param zrot rotation around z axis (in radians)
 */
void rotateTransformXYZ(float *transform,float xrot,float yrot,float zrot);
/** scales a transform by a factor in X, Y and Z axies.
 * \param transform in/out result
 * \param x scale the x axis
 * \param y scale the y axis
 * \param z scale the z axis
 */
void scaleTransform(float *transform,float x,float y,float z);
/** shift a transform by a factor in X, Y and Z axies.
 * \param transform in/out result
 * \param x the x offset
 * \param y the y offset
 * \param z the z offset
 */
void translateTransform(float *transform,float x,float y,float z);

#endif
