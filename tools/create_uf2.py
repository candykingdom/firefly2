import os
from SCons.Script import Import

Import("env")

def generate_uf2(source, target, env):
    # Path to the compiled firmware binary
    firmware_path = str(target[0])
    # Define the output path for the .uf2 file
    uf2_path = firmware_path.replace(".bin", ".uf2")
    
    # Path to the uf2conv.py tool
    uf2conv = os.path.join(env.get("PROJECT_DIR"), "tools", "uf2conv.py")
    
    # Family ID for SAMD21 is 0x68ed2b88
    family_id = "0x68ed2b88"
    
    # Base address is 0x2000
    base_address = "0x2000"
    
    print(f"Generating UF2 file: {uf2_path}")
    cmd = f'"{env.subst("$PYTHONEXE")}" "{uf2conv}" -c -f {family_id} -b {base_address} -o "{uf2_path}" "{firmware_path}"'
    env.Execute(cmd)

# Add the post-action to the build of firmware.bin
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", generate_uf2)
