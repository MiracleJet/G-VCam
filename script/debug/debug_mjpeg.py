"""
debug_mjpeg.py — MJPEG Stream Debug Tool
=========================================
Connects to an MJPEG stream at 127.0.0.1:8080, dumps headers, saves a few
JPEG frames to disk, and reports frame count / FPS.

Usage:
    python debug_mjpeg.py              # display stream info + save 5 frames
    python debug_mjpeg.py --count 20   # save 20 frames
    python debug_mjpeg.py --infinite   # stream forever, only log stats
"""

import urllib.request
import sys
import time
import os
import argparse

URL = "http://127.0.0.1:8080"


def main():
    parser = argparse.ArgumentParser(description="MJPEG Stream Debug Tool")
    parser.add_argument("--count", type=int, default=5,
                        help="Number of JPEG frames to save (default: 5)")
    parser.add_argument("--infinite", action="store_true",
                        help="Stream forever, only log stats")
    parser.add_argument("--url", default=URL,
                        help=f"MJPEG stream URL (default: {URL})")
    parser.add_argument("--out", default=".",
                        help="Output directory for saved frames")
    args = parser.parse_args()

    print(f"Connecting to {args.url} ...")

    try:
        response = urllib.request.urlopen(args.url, timeout=5)
    except Exception as e:
        print(f"ERROR: Could not connect to {args.url}")
        print(f"  {e}")
        print()
        print("Check: 1) Phone USB connected?")
        print("       2) adb forward tcp:8080 tcp:8080 ?")
        print("       3) GVCam Android app streaming?")
        sys.exit(1)

    # --- Dump response headers ---
    print()
    print("=== HTTP Response ===")
    print(f"Status: {response.status} {response.reason}")
    for key, val in response.headers.items():
        print(f"  {key}: {val}")

    content_type = response.headers.get("Content-Type", "")
    boundary = None
    for part in content_type.split(";"):
        part = part.strip()
        if part.lower().startswith("boundary="):
            boundary = part.split("=", 1)[1].strip()
            break
    if not boundary:
        print("ERROR: No boundary found in Content-Type")
        response.close()
        sys.exit(1)

    print(f"\nParsed boundary: '{boundary}'")
    print(f"Expected frame delimiter: '{boundary}' in body\n")

    # --- Parse frames ---
    marker = boundary.encode()
    buf = b""
    frame_count = 0
    saved_count = 0
    fps_start = time.time()
    fps_frames = 0
    last_jpeg_size = 0

    os.makedirs(args.out, exist_ok=True)

    print("Reading MJPEG stream...\n")

    try:
        while True:
            chunk = response.read(65536)
            if not chunk:
                print("Connection closed by server.")
                break
            buf += chunk

            # Parse all complete frames in buffer
            while True:
                pos = buf.find(marker)
                if pos == -1:
                    break

                # End of stream? (marker + "--")
                if pos + len(marker) + 2 <= len(buf) and \
                        buf[pos + len(marker)] == ord('-') and \
                        buf[pos + len(marker) + 1] == ord('-'):
                    print("End-of-stream marker received.")
                    buf = b""
                    break

                # Skip marker + optional \r\n
                hdr_start = pos + len(marker)
                if hdr_start + 2 <= len(buf) and buf[hdr_start:hdr_start + 2] == b'\r\n':
                    hdr_start += 2

                # Find end of part headers (\r\n\r\n)
                hdr_end = buf.find(b'\r\n\r\n', hdr_start)
                if hdr_end == -1:
                    break

                # Parse Content-Length
                body_start = hdr_end + 4
                cl = 0
                header_block = buf[hdr_start:hdr_end].decode("ascii", errors="ignore")
                for line in header_block.split("\r\n"):
                    if line.lower().startswith("content-length:"):
                        try:
                            cl = int(line.split(":", 1)[1].strip())
                        except ValueError:
                            pass
                        break

                if cl <= 0 or body_start + cl > len(buf):
                    break

                jpeg = buf[body_start:body_start + cl]
                last_jpeg_size = cl
                frame_count += 1
                fps_frames += 1

                # Save frames if under count
                if saved_count < args.count:
                    path = os.path.join(args.out, f"frame_{saved_count:04d}.jpg")
                    with open(path, "wb") as f:
                        f.write(jpeg)
                    print(f"  Saved {path} ({cl:,d} bytes)")
                    saved_count += 1
                elif not args.infinite and saved_count == args.count:
                    # Got enough frames
                    print(f"\nSaved {saved_count} frames. Done.")
                    response.close()
                    return

                # Advance past this frame
                next_pos = body_start + cl
                if next_pos + 2 <= len(buf) and buf[next_pos:next_pos + 2] == b'\r\n':
                    next_pos += 2
                buf = buf[next_pos:]

            # Stats every 2 seconds
            now = time.time()
            elapsed = now - fps_start
            if elapsed >= 2.0:
                fps = fps_frames / elapsed
                print(f"\r  Frame #{frame_count:6d}  |  FPS: {fps:5.1f}  |  "
                      f"JPEG: {last_jpeg_size / 1024:5.1f} KB  |  "
                      f"Buf: {len(buf) / 1024:5.1f} KB  ",
                      end="", flush=True)
                fps_frames = 0
                fps_start = now

    except KeyboardInterrupt:
        print("\n\nStopped by user.")
    finally:
        response.close()
        print(f"\nTotal frames received: {frame_count}")
        if saved_count > 0:
            abs_out = os.path.abspath(args.out)
            print(f"Saved {min(saved_count, args.count)} frames to {abs_out}/")
            print("You can open them to verify the MJPEG stream is healthy.")


if __name__ == "__main__":
    main()
