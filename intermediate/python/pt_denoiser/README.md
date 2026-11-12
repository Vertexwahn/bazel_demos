# Path Tracing U-Net Denoiser (Bazel 9.3.2 + PyTorch)

## Einrichtung & Start

1. **Lockfile generieren:**
   ```bash
   bazel run //:requirements.update
   ```

2. **Training starten:**
   ```bash
   bazel run //:train_denoiser -- \
     --dataset-dir /home/vertexwahn/dev/u-net-dataset
   ```

The trainer reads each `scene_*/reference.exr` and pairs it with the
`scene_*/noisy_spp*.exr` images. It samples random 256x256 patches so full-size
renders fit in GPU memory. By default, all available sample counts are used,
and the trained weights are saved as `pt_denoiser.pth`.

Useful options:

```bash
bazel run //:train_denoiser -- \
  --dataset-dir /path/to/dataset \
  --spp 1 4 \
  --epochs 20 \
  --batch-size 4 \
  --patch-size 256 \
  --output checkpoints/pt_denoiser.pth
```
