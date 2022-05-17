#include <cuda_runtime.h>
#include <device_launch_parameters.h>

__global__ void bgr_bgra_kernel(char* destImage,
	char* srcImage,
	int* dstPitch,
	int* dstHeigth,
	int srcPitch,
	int srcHeigth
)
{
	unsigned int index = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int stride = blockDim.x * gridDim.x;

	int resolution = srcPitch * srcHeigth;
	for (unsigned int i = index; i < resolution; i += stride) {
		destImage[4 * i + 0] = srcImage[i * 3 + 2];
		destImage[4 * i + 1] = srcImage[i * 3 + 1];
		destImage[4 * i + 2] = srcImage[i * 3 + 0];
		destImage[4 * i + 3] = 255;
	}
	return;
}
extern "C" {
	void bgr_bgra(char* rgbaBuffer,
		char* srcBuffer,
		int* dstPitch,
		int* dstHeigth,
		int srcPitch,
		int srcHeigth
	)
	{
		bgr_bgra_kernel << <320, 180 >> > (rgbaBuffer, srcBuffer, dstPitch, dstHeigth, srcPitch, srcHeigth);
		cudaDeviceSynchronize();
	}
}
