import os
import subprocess

# Set the root directory to start searching for HLSL files
root_dir = "."  # Current directory
output_dir = "../build/bin/shaders"  # Directory to store compiled shaders
output_extension = ".spv"  # Output file extension for compiled shaders

# DXC executable path (update this if necessary)
dxc_executable = "dxc"

# Shader targets map based on file naming conventions
shader_targets = {
    "vert": "vs_6_0",
    "frag": "ps_6_0",
    "comp": "cs_6_0",
    "geom": "gs_6_0",
    "tess_ctrl": "hs_6_0",
    "tess_eval": "ds_6_0",
    "mesh": "ms_6_5",
    "task": "as_6_5"
}

# Ensure the output directory exists
def ensure_output_directory():
    os.makedirs(output_dir, exist_ok=True)

# Compile a single HLSL file to SPIR-V
def compile_hlsl(file_path: str):
    # Determine the shader type by file name or extension
    file_name = os.path.basename(file_path)
    name_parts = file_name.split(".")

    # Attempt to infer shader stage from the filename (e.g., "shader.vert.hlsl")
    stage = None
    for key in shader_targets:
        if key in name_parts:
            stage = shader_targets[key]
            break

    if stage is None:
        print(f"[Skipping] Unable to determine shader stage for: {file_path}")
        return

    # Set output file name in the build/bin directory
    output_file = os.path.join(output_dir, file_name.replace(".hlsl", output_extension))

    # DXC compilation command
    command = [
        dxc_executable,
        "-T", stage,
        "-E", "main",
        "-spirv",
        "-fspv-target-env=vulkan1.3",
        "-Fo", output_file,
        file_path
    ]

    try:
        # Run the compilation command
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        ret = result.returncode
        print(f"[Compiled] {file_path} -> {output_file} {ret}")
    except subprocess.CalledProcessError as e:
        print(f"[Error] Failed to compile {file_path}\n{e.stderr}")

# Recursively find and compile all HLSL files
def compile_all_hlsl_files(directory: str):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".hlsl"):
                file_path = os.path.join(root, file)
                compile_hlsl(file_path)

if __name__ == "__main__":
    print("Starting HLSL shader compilation...")
    ensure_output_directory()
    compile_all_hlsl_files(root_dir)
    print("Shader compilation complete.")
