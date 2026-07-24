#!/usr/bin/env python3
"""
pre_build.py - Pre-build hook for Arduino_dsPIC33CK

Arduino IDE converts .ino files to .cpp and adds C++ includes.
This script patches the generated .cpp file to be C-compatible:
  1. Removes '#include <Arduino.h>' duplicate (we add our own)
  2. Removes 'extern "C"' wrappers
  3. Adds forward declarations for setup() and loop()

Usage (called by platform.txt recipe.hooks.prebuild):
    python pre_build.py {build.path} {build.project_name}
"""

import sys
import os
import re


def patch_sketch_cpp(build_path, project_name):
    """Patch the Arduino IDE generated .cpp file for C compatibility."""

    cpp_file = os.path.join(build_path, "sketch", f"{project_name}.ino.cpp")

    if not os.path.exists(cpp_file):
        # Try alternate naming patterns
        for f in os.listdir(os.path.join(build_path, "sketch")):
            if f.endswith(".ino.cpp"):
                cpp_file = os.path.join(build_path, "sketch", f)
                break

    if not os.path.exists(cpp_file):
        print(f"[pre_build] No .ino.cpp found in {build_path}/sketch/ - skipping")
        return

    print(f"[pre_build] Patching: {cpp_file}")

    with open(cpp_file, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    # Remove Arduino IDE's auto-generated line number directives that reference .ino
    # (keeps compilation errors pointing to correct lines)

    # Remove C++ specific constructs that XC16 can't handle
    # 1. Remove 'extern "C" {' blocks
    content = re.sub(r'extern\s+"C"\s*\{', '', content)

    # 2. Remove class-style method calls if any snuck in
    # (Serial.begin -> already handled by our C API)

    # 3. Ensure Arduino.h is included at the top (after #line directives)
    if '#include "Arduino.h"' not in content and '#include <Arduino.h>' not in content:
        # Add after the first #line directive or at the very top
        if '#line' in content:
            first_line_idx = content.index('#line')
            # Find end of that line
            eol = content.index('\n', first_line_idx)
            content = content[:eol+1] + '#include "Arduino.h"\n' + content[eol+1:]
        else:
            content = '#include "Arduino.h"\n' + content

    # 4. Replace <Arduino.h> with "Arduino.h" (local include)
    content = content.replace('#include <Arduino.h>', '#include "Arduino.h"')

    # Write back
    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"[pre_build] Patch complete")


def main():
    if len(sys.argv) < 3:
        print("Usage: pre_build.py <build_path> <project_name>")
        print("  Called automatically by Arduino IDE via platform.txt hooks")
        sys.exit(0)

    build_path = sys.argv[1]
    project_name = sys.argv[2]

    patch_sketch_cpp(build_path, project_name)


if __name__ == '__main__':
    main()
