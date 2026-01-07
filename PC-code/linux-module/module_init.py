import subprocess
import os
from typing import Tuple

MODULE_NAME = "scan"
DEVICE_PATH = "/dev/scan"


class LinuxModule:
    def __init__(self, module_name, device_path):
        self.__module_name = module_name
        self._device_path: str = device_path
        self.__module_file = f"{self.__module_name}.ko"
        print(self.__module_name, self.__module_file)

    def _run_cmd(self, command: str, need_sudo=False) -> Tuple[bool, str, str]:
        if need_sudo and os.geteuid() != 0:
            command = ["sudo"] + command

        result = subprocess.run(
            command,
            capture_output=True,
            text=True
        )
        return result.returncode == 0, result.stdout, result.stderr
    
    def _is_ko(self) -> bool:
        return os.path.exists(self.__module_file)

    def _compile_module(self) -> Tuple[bool, str, str]:
        success, out, err = self._run_cmd(["make", "clean"])
        success, out, err = self._run_cmd(["make"])

        if success and os.path.exists(self.__module_file):
            print(f"Module {self.__module_file} compiled")
        else:
            print(f"Module {self.__module_file} compilation failed:")
            print(err)
            exit(1)

        return success, out, err

    def is_module_loaded(self) -> bool:
        success, out, err = self._run_cmd(["lsmod"])
        return MODULE_NAME in out

    def _loading_module(self) -> bool:
        success, out, err = self._run_cmd(["insmod", self.__module_file], need_sudo=True)
        if not success:
            print(f"Module {self._device_path} failed to load: {err}")
            exit(1)
        return success

    def _unloading_module(self) -> bool:
        success, out, err = self._run_cmd(["rmmod", self.__module_file], need_sudo=True)
        if not success:
            print(f"Module {self._device_path} failed to unload: {err}")
            exit(1)
        return success
    
    def _check_device(self) -> bool:
        if(os.path.exists(path=self._device_path) == False):
            raise FileNotFoundError(f"Device {self._device_path} does not exist")
        return True
    
    def _kernel_log_tail(self, num_lines: int):
        success, out, err = self._run_cmd(["dmesg"], False)

        if not success:
            print(f"Failed to read dmesg: {err}")
            return []

        lines = out.splitlines()
        messages = []
        buffer = []

        for line in lines:
            if self.__module_name.lower() in line.lower():
                if buffer:
                    messages.append("\n".join(buffer))
                    buffer = []
                buffer.append(line)
            else:
                if buffer:
                    buffer.append(line)

        if buffer:
            messages.append("\n".join(buffer))

        tail = messages[-num_lines:]
        for msg in tail:
            print(msg)
        return tail

def main():
    module = LinuxModule(module_name=MODULE_NAME, device_path=DEVICE_PATH)
    if(module._is_ko() == False):
        success, out, err = module._compile_module()
        if(success):
            print("Module successfully compiled\n")
    if(module.is_module_loaded() == False):
        success, out, err = module._loading_module()
        if(success):
            print("Module successfully loaded\n")

    if(module._check_device()== False):
        raise FileNotFoundError ("Character device /dev/scan does not exist\n")

if __name__ == "__main__":
    main()