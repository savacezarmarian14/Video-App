# Video-App

A real-time video processing application written in C using GStreamer.

It captures video from the laptop camera, scales it, inverts the colours,
optionally flips it, encodes it to H.264, and then shows it on screen while
saving it to a file at the same time.

---

## Requirements

- GStreamer 1.20 or newer
- gcc, make, pkg-config

On Ubuntu/Debian:

```bash
sudo apt install build-essential pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly gstreamer1.0-libav \
    frei0r-plugins
```

Developed and tested on Ubuntu 24.04 with GStreamer 1.24.2.

---

## Build

```bash
make
```

This builds every `.c` file into a binary with the same name. To clean:

```bash
make clean
```

---

## Run

```bash
./video-app [OPTIONS]
```

| Option | Short | Default | Description |
|---|---|---|---|
| `--width N` | `-w` | `320` | Output width |
| `--height N` | | `240` | Output height |
| `--flip STR` | `-f` | `none` | Flip method |
| `--device PATH` | `-d` | `/dev/video0` | Camera device |
| `--record PATH` | `-r` | `output.mkv` | Where to save the video |

Flip values: `none`, `clockwise`, `rotate-180`, `counterclockwise`,
`horizontal-flip`, `vertical-flip`.

Examples:

```bash
./video-app
./video-app -w 640 --height 480 -f horizontal-flip
./video-app -d /dev/video2 -r /tmp/capture.mkv
```

Press **Ctrl+C** to stop. The application shuts down cleanly so the saved file
is complete and playable.

To check the result:

```bash
gst-play-1.0 output.mkv
```

---

## Pipeline

```
camera
  |
set format (YUY2, 640x480, 30 fps)
  |
scale
  |
invert colours
  |
flip
  |
encode to H.264
  |
 [tee]  -- splits the stream in two
  |
  +--> decode --> show on screen
  |
  +--> save to file
```

---

## Design decisions

**Format is fixed at the camera.** The camera only gives 30 fps up to 640x480,
so the format and frame rate are set explicitly at the start. This is how the
real-time requirement is met.

**Scaling comes first.** Everything after it works on fewer pixels, which keeps
the pipeline fast.

**Encoding comes last.** The filters work on raw video, so they have to run
before the video is compressed.

**The stream is encoded only once.** The task asks for H.264 output shown on
screen, which means encoding and then decoding again for the display. Instead
of encoding a second time for the file, the stream is split after encoding:
one branch decodes it for the screen, the other saves it directly.

**Matroska (.mkv) instead of MP4.** MP4 can end up unplayable if the program is
stopped abruptly. Matroska handles that much better.

---

## Development steps

Each stage of the work is kept in a separate file:

| File | What it adds |
|---|---|
| `01-preview.c` | Camera to screen |
| `02-caps.c` | Fixed format and frame rate |
| `03-scale.c` | Scaling |
| `04-invert.c` | Colour inversion |
| `05-flip.c` | Flip and command line options |
| `6-encodeH264.c` | H.264 encoding |
| `video-app.c` | Saving to file |

`video-app.c` is the final application.

---

## Notes

- The camera is limited to 640x480 at 30 fps in this format. Higher resolutions
  are possible but the frame rate drops.
- Recording is always on, writing to `output.mkv` unless `--record` is used.
- Only video is handled, no audio.