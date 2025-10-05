import sys
import subprocess
import importlib.metadata

required = {"demucs", "crepe", "librosa", "soundfile", "tensorflow", "onnxruntime"}
installed = {dist.metadata["Name"].lower() for dist in importlib.metadata.distributions()}
missing = required - installed

if missing:
    python = sys.executable
    subprocess.check_call([python, "-m", "pip", "install", *missing], stdout=subprocess.DEVNULL)

import argparse
import crepe
import librosa
import soundfile
import os
import shutil
import demucs.separate

def separate_vocals(input: str, output: str="vocals.wav"):
    output_dir = os.path.dirname(output)
    demucs.separate.main(["--two-stems=vocals", "-o", output_dir, input])

    demucs_folder = os.path.join(output_dir, "htdemucs")
    dest_folder = os.listdir(demucs_folder)[0]
    vocal_path = os.path.join(demucs_folder, dest_folder, "vocals.wav")

    if (os.path.exists(output)):
        os.remove(output)

    os.rename(vocal_path, output)
    shutil.rmtree(demucs_folder)
    return output

def chop_vocals(input: str, output: str="chops", name: str="", threshold: float=0.5, min_duration: float=0.2, min_rms: float=0.01):
    audio, sr = librosa.load(input, sr=16000)
    time, freq, conf, act = crepe.predict(audio, sr, viterbi=True)

    voiced = [i for i, c in enumerate(conf) if c > threshold]
    to_samples = lambda frame: int(frame * 0.01 * sr)

    regions = []
    if voiced:
        start = voiced[0]
        end = start
        for frame in voiced[1:]:
            if frame - end > 3:
                regions.append((to_samples(start), to_samples(end)))
                start = frame
            end = frame
        regions.append((to_samples(start), to_samples(end)))

    output_basename = os.path.splitext(os.path.basename(name if name else input))[0]
    output_dir = os.path.join(output, f"{output_basename} chops")

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    os.makedirs(output_dir, exist_ok=True)
    for i, (start, end) in enumerate(regions):
        chop = audio[start:end]

        if len(chop) < int(min_duration * sr):
            continue

        rms = librosa.feature.rms(y=chop).mean()
        if rms < min_rms:
            continue

        trim, _ = librosa.effects.trim(chop)

        soundfile.write(os.path.join(output_dir, f"chop{i}.wav"), trim, sr)
    return output_dir

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Vocal Chopper")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--separate", action="store_true")
    group.add_argument("--chop", action="store_true")

    parser.add_argument("-i", "--input")
    parser.add_argument("-o", "--output")
    parser.add_argument("-n", "--name")

    parser.add_argument("--threshold", type=float, default=0.5)
    parser.add_argument("--min-duration", type=float, default=0.2)
    parser.add_argument("--min-rms", type=float, default=0.01)

    args = parser.parse_args()

    if args.separate:
        output = separate_vocals(args.input, args.output)
        print(output)
    elif args.chop:
        output = chop_vocals(args.input, args.output, args.name, args.threshold, args.min_duration, args.min_rms)
        print(output)