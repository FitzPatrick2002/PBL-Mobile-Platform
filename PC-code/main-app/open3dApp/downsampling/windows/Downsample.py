import os

class DownsampleWindows:
    def __init__(self):
        self.__percent = 0
        self.__line = 1
        self.__mode = 1

    def _downsampling_per(self):
        pass

    def _downsampling_line(self):
        pass

#     static
#     char * downsampling_per(char * output, const
#     char * input, size_t
#     input_len, size_t * output_len, size_t
#     tot_lines){
#         size_t
#     curr_line = 0;
#     size_t
#     bytes_so_far = 0;
#     size_t
#     out_pos = 0;
#     size_t
#     i;
#     for (i = 0; i < input_len; i++) {
#     if (input[i] == '\n') {
#     size_t line_len = i - bytes_so_far + 1;
#
#     if ((curr_line * 100) >= (dsmp_line_per * tot_lines)) {
#     if (out_pos + line_len > * output_len) {
#     pr_err("Scan: Buffer overflow detected!\n");
#     vfree(output);
#     return NULL;
#     }
#
#     memcpy(output + out_pos, input + bytes_so_far, line_len);
#     out_pos += line_len;
#     }
#
#
#     curr_line + +;
#     bytes_so_far = i + 1;
#     }
#     }
#
#     if (bytes_so_far < input_len){// some line left
#
#     if ((curr_line * 100) >= (dsmp_line_per * tot_lines)) {
#
#     size_t line_len = input_len - bytes_so_far;
#
#     if (out_pos + line_len > * output_len) {
#     pr_err("Scan: Buffer overflow detected!\n");
#     vfree(output);
#     return NULL;
#     }
#
#     memcpy(output + out_pos, input + bytes_so_far, line_len);
#     out_pos += line_len;
#     }
#     }
#     return output;
#     }
#
#     static
#     char * downsampling_line(char * output, const
#     char * input, size_t
#     input_len, size_t * output_len, size_t
#     tot_lines){
#     size_t
#     curr_line = 0;
#     size_t
#     bytes_so_far = 0;
#     size_t
#     out_pos = 0;
#     size_t
#     i;
#
#     if (dsmp_line_num > tot_lines | | dsmp_line_num <= 0){
#     pr_err("Scan: The desampling line number is incorrect. (RANGE 0 - %zu)\n", tot_lines);
#     return NULL;
#     }
#
#     if (dsmp_mode == 1){
#     for (i = 0; i < input_len; i++) {
#     if (input[i] == '\n') {
#     size_t line_len = i - bytes_so_far + 1;
#
#     if (curr_line %dsmp_line_num != 0) {
#     if (out_pos + line_len > * output_len) {
#     pr_err("Scan: Buffer overflow detected!\n");
#     vfree(output);
#     return NULL;
#     }
#
#     memcpy(output + out_pos, input + bytes_so_far, line_len);
#     out_pos += line_len;
#     }
#
#
#     curr_line + +;
#     bytes_so_far = i + 1;
#     }
#     }
#     } else if (dsmp_mode == 2){
#     for (i = 0; i < input_len; i++) {
#     if (input[i] == '\n') {
#     size_t line_len = i - bytes_so_far + 1;
#
#     if (curr_line %dsmp_line_num == 0) {
#     if (out_pos + line_len > * output_len) {
#     pr_err("Scan: Buffer overflow detected!\n");
#     vfree(output);
#     return NULL;
#
# }
#
# memcpy(output + out_pos, input + bytes_so_far, line_len);
# out_pos += line_len;
# }
#
#
# curr_line + +;
# bytes_so_far = i + 1;
# }
# }
# } else {
#     pr_err("Scan: Incorrect desampling mode in desampling line method: %u mode\n", dsmp_mode);
# return NULL;
# }
#
# if (bytes_so_far < input_len){// some line left
# if (dsmp_mode == 1){
#
# if (curr_line %dsmp_line_num != 0) {
#
# size_t line_len = input_len - bytes_so_far;
#
# if (out_pos + line_len > * output_len) {
# pr_err("Scan: Buffer overflow detected!\n");
# vfree(output);
# return NULL;
# }
#
# memcpy(output + out_pos, input + bytes_so_far, line_len);
# out_pos += line_len;
# }
# } else if (dsmp_mode == 2){
# if (curr_line %dsmp_line_num == 0) {
#
# size_t line_len = input_len - bytes_so_far;
#
# if (out_pos + line_len > * output_len) {
# pr_err("Scan: Buffer overflow detected!\n");
# vfree(output);
# return NULL;
# }
#
# memcpy(output + out_pos, input + bytes_so_far, line_len);
# out_pos += line_len;
# }
# } else {
#     pr_err("Scan: Incorrect desampling mode in desampling line method: %u mode\n", dsmp_mode);
# return NULL;
# }
#
# }
# return output;
# }

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

        if(self.__mode == 0):
            self._downsampling_per()
        else:
            self._downsampling_line()

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