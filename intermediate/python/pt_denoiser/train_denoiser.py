import argparse
import os
from pathlib import Path

import numpy as np
import OpenEXR
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader


DEFAULT_DATASET_DIR = "/home/vertexwahn/dev/u-net-dataset"


class ConvBlock(nn.Module):
    def __init__(self, in_c, out_c):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(in_c, out_c, kernel_size=3, padding=1),
            nn.LeakyReLU(0.1, inplace=True),
            nn.Conv2d(out_c, out_c, kernel_size=3, padding=1),
            nn.LeakyReLU(0.1, inplace=True),
        )

    def forward(self, x):
        return self.conv(x)


class PathTracingUNet(nn.Module):
    def __init__(self, channels=3):
        super().__init__()
        self.enc1 = ConvBlock(channels, 32)
        self.enc2 = ConvBlock(32, 64)
        self.enc3 = ConvBlock(64, 128)
        self.pool = nn.MaxPool2d(2)

        self.bottleneck = ConvBlock(128, 256)

        self.up3 = nn.ConvTranspose2d(256, 128, kernel_size=2, stride=2)
        self.dec3 = ConvBlock(256, 128)

        self.up2 = nn.ConvTranspose2d(128, 64, kernel_size=2, stride=2)
        self.dec2 = ConvBlock(128, 64)

        self.up1 = nn.ConvTranspose2d(64, 32, kernel_size=2, stride=2)
        self.dec1 = ConvBlock(64, 32)

        self.final = nn.Conv2d(32, channels, kernel_size=1)

    def forward(self, x):
        e1 = self.enc1(x)
        e2 = self.enc2(self.pool(e1))
        e3 = self.enc3(self.pool(e2))

        b = self.bottleneck(self.pool(e3))

        d3 = self.dec3(torch.cat([self.up3(b), e3], dim=1))
        d2 = self.dec2(torch.cat([self.up2(d3), e2], dim=1))
        d1 = self.dec1(torch.cat([self.up1(d2), e1], dim=1))

        return x + self.final(d1)


def load_exr_rgb(path):
    exr_file = OpenEXR.File(str(path))
    channels = exr_file.channels()
    channel_names = {
        name.rsplit(".", 1)[-1].upper(): channel for name, channel in channels.items()
    }

    if all(name in channel_names for name in ("R", "G", "B")):
        image = np.stack(
            [channel_names[name].pixels for name in ("R", "G", "B")], axis=-1
        )
    elif "RGB" in channel_names:
        image = channel_names["RGB"].pixels
        if image.ndim != 3 or image.shape[-1] < 3:
            raise ValueError(f"Expected an RGB channel in {path}, got {image.shape}")
        image = image[..., :3]
    else:
        raise ValueError(
            f"Expected R, G, and B channels in {path}; found {list(channels)}"
        )

    if image.ndim != 3 or image.shape[-1] != 3:
        raise ValueError(f"Expected a three-channel image in {path}, got {image.shape}")
    if not np.isfinite(image).all():
        raise ValueError(f"EXR image contains non-finite pixel values: {path}")
    return torch.from_numpy(np.ascontiguousarray(image.transpose(2, 0, 1)))


class PathTracingDataset(Dataset):
    def __init__(self, root, patch_size=256, patches_per_image=64, spp=None):
        self.patch_size = patch_size
        self.samples = []
        references = {}

        for scene_dir in sorted(Path(root).glob("scene_*")):
            noisy_paths = sorted(scene_dir.glob("noisy_spp*.exr"))
            if spp is not None:
                noisy_paths = [
                    path
                    for path in noisy_paths
                    if self._samples_per_pixel(path) in spp
                ]
            if not noisy_paths:
                continue

            reference_path = scene_dir / "reference.exr"
            if not reference_path.is_file():
                raise FileNotFoundError(
                    f"Missing reference image for noisy images in {scene_dir}"
                )
            clean = references.get(reference_path)
            if clean is None:
                clean = load_exr_rgb(reference_path)
                references[reference_path] = clean

            for noisy_path in noisy_paths:
                noisy = load_exr_rgb(noisy_path)
                if noisy.shape != clean.shape:
                    raise ValueError(
                        f"Image dimensions differ for {noisy_path} and "
                        f"{reference_path}: {tuple(noisy.shape)} != "
                        f"{tuple(clean.shape)}"
                    )
                if min(noisy.shape[-2:]) < patch_size:
                    raise ValueError(
                        f"Patch size {patch_size} exceeds image dimensions "
                        f"{tuple(noisy.shape[-2:])} in {noisy_path}"
                    )
                self.samples.append((noisy, clean))

        if not self.samples:
            selection = f" for spp {sorted(spp)}" if spp is not None else ""
            raise FileNotFoundError(
                f"No matching scene/reference/noisy EXR pairs found in {root}"
                f"{selection}"
            )
        self.patches_per_image = patches_per_image

    @staticmethod
    def _samples_per_pixel(path):
        try:
            return int(path.stem.removeprefix("noisy_spp"))
        except ValueError as error:
            raise ValueError(f"Invalid noisy image filename: {path}") from error

    def __len__(self):
        return len(self.samples) * self.patches_per_image

    def __getitem__(self, idx):
        noisy, clean = self.samples[idx % len(self.samples)]
        height, width = noisy.shape[-2:]
        top = int(torch.randint(height - self.patch_size + 1, (1,)))
        left = int(torch.randint(width - self.patch_size + 1, (1,)))
        patch = (
            ...,
            slice(top, top + self.patch_size),
            slice(left, left + self.patch_size),
        )
        return noisy[patch], clean[patch]


def parse_args():
    parser = argparse.ArgumentParser(description="Train a path-tracing EXR denoiser.")
    parser.add_argument("--dataset-dir", default=DEFAULT_DATASET_DIR)
    parser.add_argument("--output", default="pt_denoiser.pth")
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=8)
    parser.add_argument("--patch-size", type=int, default=256)
    parser.add_argument("--patches-per-image", type=int, default=64)
    parser.add_argument(
        "--spp",
        type=int,
        nargs="+",
        help="Only train on these samples-per-pixel levels; default uses all levels.",
    )
    args = parser.parse_args()
    if args.patch_size <= 0 or args.patch_size % 8 != 0:
        parser.error("--patch-size must be a positive multiple of 8")
    if args.epochs <= 0 or args.batch_size <= 0 or args.patches_per_image <= 0:
        parser.error("--epochs, --batch-size, and --patches-per-image must be positive")
    return args


def train(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Training on: {device}")

    dataset = PathTracingDataset(
        args.dataset_dir,
        patch_size=args.patch_size,
        patches_per_image=args.patches_per_image,
        spp=set(args.spp) if args.spp else None,
    )
    dataloader = DataLoader(dataset, batch_size=args.batch_size, shuffle=True)
    print(f"Loaded {len(dataset.samples)} noisy/reference pairs")

    model = PathTracingUNet(channels=3).to(device)
    criterion = nn.L1Loss()
    optimizer = optim.Adam(model.parameters(), lr=1e-4)

    for epoch in range(args.epochs):
        model.train()
        total_loss = 0.0

        for noisy, clean in dataloader:
            noisy, clean = noisy.to(device), clean.to(device)

            optimizer.zero_grad()
            output = model(noisy)
            loss = criterion(output, clean)

            loss.backward()
            optimizer.step()

            total_loss += loss.item()

        avg_loss = total_loss / len(dataloader)
        print(f"Epoch [{epoch + 1}/{args.epochs}] - Loss: {avg_loss:.5f}")

    work_dir = os.environ.get("BUILD_WORKING_DIRECTORY", ".")
    save_path = Path(args.output)
    if not save_path.is_absolute():
        save_path = Path(work_dir) / save_path
    save_path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), save_path)
    print(f"Training complete. Model saved to: {save_path}")


if __name__ == "__main__":
    train(parse_args())
