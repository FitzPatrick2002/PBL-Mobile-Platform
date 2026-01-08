import open3dApp.downsampling.linux.module_init as mi
import time
import os

#some notes
#You cannot read kernel logs nor use the method get_info as they require admin priviledges
class DownsampleModule(mi.LinuxModule):
    def __init__(self, module_name = mi.MODULE_NAME, device_path = mi.DEVICE_PATH):
        mi.LinuxModule.__init__(self, module_name, device_path)
        #for downsampling
        self.__percent = 0
        self.__line = 1
        self.__mode = 1

    def _set_input_path(self, input_path: str):
        if(len(input_path)==0):
            raise Exception(
                f"The input path cannot be empty"
            )
        
        if(os.path.exists(path=input_path) == False):
            raise Exception(
                f"The input path {input_path} does not exist"
            )

        if(os.path.getsize(input_path)==0):
            raise Exception(
                f"The input path {input_path} cannot be empty"
            )
        
        try:

            self.__input_path = input_path
            abs_path = os.path.abspath(input_path)
            
            with open(self._device_path, 'w') as f:
                f.write(f"INPUT:{abs_path}")
        except Exception as e:
            print(f"Could not set the input path: {input_path}\n{e}")

    def _get_input_path(self):
        return self.__input_path

    def _set_output_path(self, output_path: str):
        if(len(output_path)==0):
            raise Exception(
                f"The output path cannot be empty"
            )
        
        if(os.path.exists(path=output_path) == False):
            print("New output file will be created")
        else:
            print("Overwritting existing output file")
        
        self.__output_path = output_path
        abs_path = os.path.abspath(output_path)
        try:
            with open(self._device_path, 'w') as f:
                f.write(f"OUTPUT:{abs_path}")
        except Exception as e:
            print(f"Could not set the output path: {output_path}\n{e}")

    def _get_output_path(self) -> str:
        return self.__output_path
    
    def _set_percent(self, percent: int):
        if(percent > 100 | percent < 0):
            print(f"The percent cannot have this value: {percent}.\nUse range [0-100]")
            return
        try:
            self.__percent = percent
            with open(self._device_path, 'w') as f:
                f.write(f"PER:{percent}")
        except Exception as e:
            print(f"Could not set the percentage: {percent}\n{e}")
        
    def _get_percent(self) -> int:
        return self.__percent

    def _set_lines(self, lines: int):
        if(lines < 0):
            print("The number of lines should be positive and no longer than the number of lines in the input file")
        self.__line = lines
        try:
            self.__line = lines
            with open(self._device_path, 'w') as f:
                f.write(f"LINE:{lines}")
        except Exception as e:
            print(f"Could not set the lines: {lines}\n{e}")

    def _get_lines(self) -> int:
        return self.__line
    
    def _set_mode(self, mode: int):
        if(mode < 0 | mode > 2):
            print("The mode has to be in range [0-2]")
            return
        try:
            self.__mode = mode
            with open(self._device_path, 'w') as f:
                f.write(f"MODE:{mode}")
        except Exception as e:
            print(f"Could not set the mode: {mode}\n{e}")

    def _get_mode(self) -> int:
        return self.__mode
    
    def _run(self):
        if(len(self.__input_path) ==0 | len(self.__output_path) == 0):
            raise Exception(
                f"The paths cannot be empty"
            )
        try:
            with open(self._device_path, 'w') as f:
                f.write(f"RUN")

            time.sleep(0.1) #can be deleted
        except Exception as e:
            print(f"Something went wrong when running: {e}")

    def _get_info(self):
        try:
            with open(self._device_path, 'w') as f:
                f.write(f"INFO")
        except Exception as e:
            print(f"Could not get the info\n{e}")
        return self._kernel_log_tail(2)

    def downsample(self):
        print("In Linux downsample")
#
# def main():
#     module = DownsampleModule()
#     module._set_input_path("input.txt")
#     module._set_output_path("output.txt")
#     module._set_mode(2)
#     module._set_lines(5)
#     module._run()
#
# if __name__ == "__main__":
#     main()