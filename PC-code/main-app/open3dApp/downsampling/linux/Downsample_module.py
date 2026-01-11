"""
This module provides utilities regarding the Linux kernel module scan.ko.
"""
import open3dApp.downsampling.linux.module_init as mi
import time
import os

class DownsampleModule(mi.LinuxModule):
    """Configures and runs the Linux kernel module called scan for downsampling the scans.
       You cannot read kernel logs nor use the method get_info as they require admin privileges."""
    def __init__(self,
                 module_name = mi.MODULE_NAME,
                 device_path = mi.DEVICE_PATH):
        """Initialization of the class, default values for the parameters.

        Keyword arguments:
            module_name -- the name of the module (in here 'scan')
            device_path -- the path to the character device (in here /dev/scan)
        """
        mi.LinuxModule.__init__(self, module_name, device_path)

        """For downsampling."""
        self.__percent = 0
        self.__line = 1
        self.__mode = 1

    def _set_input_path(self, input_path: str):
        """Sets the path to the input file.

        Keyword arguments:
            input_path -- the path to the input file.
        """
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
        """Returns the path to the input file.

        Returns:
            input_path -- the str value.
        """
        return self.__input_path

    def _set_output_path(self, output_path: str):
        """Sets the path to the output file.

        Keyword arguments:
            output_path -- the path to the output file.
        """
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
        """Returns the path to the output file.

        Returns:
            output_path -- the str value.
        """
        return self.__output_path
    
    def _set_percent(self, percent: int):
        """Sets the percentage value for mode 0.

        Keyword arguments:
            percent -- the integer value from 0 to 100.
        """
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
        """Returns the percentage value from mode 0.

        Returns:
            percent -- the integer percentage value from 0 to 100.
        """
        return self.__percent

    def _set_lines(self, lines: int):
        """Sets the number of lines for modes 1 or 2.

        Keyword arguments:
            lines -- the number of lines to be filtered or kept, min. 1.
        """
        if(lines < 0):
            print("The number of lines should be positive and no longer than the number of lines in the input file")
        try:
            self.__line = lines
            with open(self._device_path, 'w') as f:
                f.write(f"LINE:{lines}")
        except Exception as e:
            print(f"Could not set the lines: {lines}\n{e}")

    def _get_lines(self) -> int:
        """Returns the number of lines for modes 1 or 2.

        Returns:
            line -- the integer value, min. 1.
        """
        return self.__line
    
    def _set_mode(self, mode: int):
        """Sets the mode of operation for the module:
        0 - percentage of lines will get filtered,
        1 - each x line is kept,
        2 - each x line is filtered out.

        Keyword arguments:
            mode -- the integer value [0-2].
        """
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
        """Returns the mode of operation used by the module.

        Returns:
            mode -- integer value from 0 to 2.
        """
        return self.__mode
    
    def _run(self):
        """Runs the module with set arguments."""
        if(len(self.__input_path) ==0 | len(self.__output_path) == 0):
            raise Exception(
                f"The paths cannot be empty"
            )
        try:
            with open(self._device_path, 'w') as f:
                f.write(f"RUN")

            """Small delay to ensure the downsampling has been completed."""
            time.sleep(0.1)
        except Exception as e:
            print(f"Something went wrong when running: {e}")

    def _get_info(self):
        """Returns the info from the module itself, using the INFO command.

        Returns:
            kernel_log_tail -- two last lines of kernel log (hopefully) containing our info.
        """
        try:
            with open(self._device_path, 'w') as f:
                f.write(f"INFO")
        except Exception as e:
            print(f"Could not get the info\n{e}")
        return self._kernel_log_tail(2)

    def downsample(self, mode, percent, lines, input_path, output_path):
        """Sets all the parameters and runs the downsampling algorithm from the module.

        Keyword arguments:
            mode -- the mode of operation from 0 to 2,
            percent -- the percentage of filtered out lines from 0 to 100,
            input_path -- the input path,
            output_path -- the output path.
        """
        self._set_mode(mode)
        self._set_lines(lines)
        self._set_percent(percent)
        self._set_input_path(input_path)
        self._set_output_path(output_path)
        self._run()
        print("In Linux downsample")

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