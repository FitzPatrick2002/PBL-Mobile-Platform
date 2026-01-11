"""
This module contains utilities regarding the downsampling algorithm, version for non-Linux OS.
"""
import os
import csv

class DownsampleDefault:
    """Downsampling algorithm and file parsing."""
    def __init__(self):
        """Initializing the default values for the algorithm."""
        self.__percent = 0
        self.__line = 1
        self.__mode = 1

    def _downsampling_per(self):
        """Performs the downsampling algorithm for a percent of filtered-out lines."""
        csv_result = []
        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file)

            """Count rows."""
            count = 0
            for row in csv_reader:
                count += 1

        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file)

            """Iterate through each row in the CSV."""
            index = 0
            for row in csv_reader:
                if ((index * 100) >= (self.__percent * count)):
                    csv_result.append(row)
                index += 1

        with open(self.__output_abs_path, 'w', newline = '') as file:
            csv_writer = csv.writer(file)
            csv_writer.writerows(csv_result)

    def _downsampling_line(self):
        """Performs the downsampling algorithm for the number of lines.
        Regardless of mode:
            1 - x line is ignored,
            2 - x line is kept.
        """
        csv_result = []
        with open(self.__input_abs_path, 'r') as file:
            csv_reader = csv.reader(file)

            """Iterate through each row in the CSV."""
            index = 0
            if self.__mode == 1:
                """1 - x line is ignored."""
                for row in csv_reader:
                    if(index % self.__line != 0):
                        csv_result.append(row)
                    index += 1
            else:
                """2 - x line is kept."""
                for row in csv_reader:
                    if(index % self.__line == 0):
                        csv_result.append(row)
                    index += 1

        with open(self.__output_abs_path, 'w', newline = '') as file:
            csv_writer = csv.writer(file)
            csv_writer.writerows(csv_result)

    def _set_input_path(self, input_path: str):
        """Sets the input path.

        Keyword arguments:
            input_path -- input path.
        """

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
        """Returns the input path.

        Returns:
            input_path -- str value.
        """
        return self.__input_path

    def _set_output_path(self, output_path: str):
        """Sets the output path.

        Keyword arguments:
            output_path -- output path.
        """
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
        """Returns the output path.

        Returns:
            output_path -- str value.
        """
        return self.__output_path

    def _set_percent(self, percent: int):
        """Sets the percent value for mode 0.

        Keyword arguments:
            percent -- integer value ranging from 0 to 100.
        """
        if (percent > 100 or percent < 0):
            print(f"The percent cannot have this value: {percent}.\nUse range [0-100]")
            return

        self.__percent = percent

    def _get_percent(self) -> int:
        """Returns the percent value.

        Returns:
            percent -- integer value ranging from 0 to 100.
        """
        return self.__percent

    def _set_lines(self, lines: int):
        """Sets the number of lines for modes 1 and 2.

        Keyword arguments:
            lines - integer value, min. 1.
        """
        if (lines < 0):
            print("The number of lines should be positive and no longer than the number of lines in the input file")
        self.__line = lines

    def _get_lines(self) -> int:
        """Returns the number of lines for mode 1 or 2.

        Returns:
            line -- integer value, min. 1.
        """
        return self.__line

    def _set_mode(self, mode: int):
        """Sets the mode of operation for the algorithm.

        Keyword arguments:
            mode -- integer value in 0,1,2.
                    0 - percent mode,
                    1 - x line is ignored,
                    2 - x line is kept.
        """
        if (mode < 0 or mode > 2):
            print("The mode has to be in range [0-2]")
            return
        self.__mode = mode

    def _get_mode(self) -> int:
        """Returns the operation mode.

        Returns:
            mode -- integer value ranging from 0 to 2.
        """
        return self.__mode

    def _run(self):
        """Runs the adequate operation mode."""
        if (len(self.__input_path) == 0 or len(self.__output_path) == 0):
            raise Exception(
                f"The paths cannot be empty"
            )

        if(self.__mode == 0):
            self._downsampling_per()
        else:
            self._downsampling_line()

    def _get_info(self):
        """Prints the information about all the parameters."""
        print(f"Mode: {self.__mode}\n")
        print(f"Percent: {self.__percent}\n")
        print(f"Lines: {self.__line}\n")
        print(f"Input path: {self.__input_path}\n")
        print(f"Output path: {self.__output_path}\n")

    def downsample(self, mode, percent, lines, input_path, output_path):
        """Sets all the needed parameters and runs the appropriate algorithm based on the operation mode.

        Keyword arguments:
            mode -- integer value in 0,1,2.
                    0 - percent mode,
                    1 - x line is ignored,
                    2 - x line is kept,
            percent -- integer value ranging from 0 to 100, used in mode 0,
            lines -- integer value, min. 1, used in modes 1 and 2,
            input_path -- the input path to the file with the scan to be downsampled,
            output_path -- the output path for the downsampled scan to be stored at.
        """
        self._set_mode(mode)
        self._set_lines(lines)
        self._set_percent(percent)
        print(f"Downsample: downsample(): Basics set, incoming: {input_path}")

        try:
            self._set_input_path(input_path)
            self._set_output_path(output_path)
        except Exception as e:
            print(f"Downsample: downsample(): Exception occured: {e}")

        print(f"Downsample: downsample(): All set")
        self._run()
        print("In windows downsample")

# def main():
#     module = DownsampleWindows()
#     module.downsample(0, 20, 3, "input.csv", "output2.csv")
#
# if __name__ == "__main__":
#     main()