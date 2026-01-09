import os

class DownsampleWindows:
    def __init__(self):
        self.__percent = 0
        self.__line = 1
        self.__mode = 1

    def _set_input_path(self, input_path: str):
        if (len(input_path) == 0):
            raise Exception(
                f"The input path cannot be empty"
            )

        if (os.path.exists(path=input_path) == False):
            raise Exception(
                f"The input path {input_path} does not exist"
            )

        if (os.path.getsize(input_path) == 0):
            raise Exception(
                f"The input path {input_path} cannot be empty"
            )

        self.__input_path = input_path
        self.__input_abs_path = os.path.abspath(input_path)

    def _get_input_path(self):
        return self.__input_path

    def _set_output_path(self, output_path: str):
        if (len(output_path) == 0):
            raise Exception(
                f"The output path cannot be empty"
            )

        if (os.path.exists(path=output_path) == False):
            print("New output file will be created")
        else:
            print("Overwritting existing output file")

        self.__output_path = output_path
        self.__output_abs_path = os.path.abspath(output_path)

    def _get_output_path(self) -> str:
        return self.__output_path

    def _set_percent(self, percent: int):
        if (percent > 100 | percent < 0):
            print(f"The percent cannot have this value: {percent}.\nUse range [0-100]")
            return

        self.__percent = percent


    def _get_percent(self) -> int:
        return self.__percent

    def _set_lines(self, lines: int):
        if (lines < 0):
            print("The number of lines should be positive and no longer than the number of lines in the input file")
        self.__line = lines

    def _get_lines(self) -> int:
        return self.__line

    def _set_mode(self, mode: int):
        if (mode < 0 | mode > 2):
            print("The mode has to be in range [0-2]")
            return
        self.__mode = mode

    def _get_mode(self) -> int:
        return self.__mode

    def _run(self):
        if (len(self.__input_path) == 0 | len(self.__output_path) == 0):
            raise Exception(
                f"The paths cannot be empty"
            )
        #implement algorithms

    def _get_info(self):
        print(f"Mode: {self.__mode}\n")
        print(f"Percent: {self.__percent}\n")
        print(f"Lines: {self.__line}\n")
        print(f"Input path: {self.__input_path}\n")
        print(f"Output path: {self.__output_path}\n")

    def downsample(self, mode, percent, lines, current_scan, temp_folder):
        self._set_mode(mode)
        self._set_lines(lines)
        self._set_percent(percent)
        self._set_input_path(current_scan)
        self._set_output_path(temp_folder)
        self._run()
        print("In windows downsample")