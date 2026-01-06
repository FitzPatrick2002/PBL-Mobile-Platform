#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

static struct mutex scan_lock;

#define MAX_PATH_LEN 256

static size_t scan_len;

static char input_path[MAX_PATH_LEN];
static char output_path[MAX_PATH_LEN];

static int last_status;
static char status_msg[128];

static unsigned int dsmp_line_per = 0; //desampling line number, cannot be 0
static unsigned int dsmp_line_num = 1 ; //each dsmp_line_num line is ignored
static unsigned int dsmp_mode = 1; //0 - percentage mode, 1 - x line is ignored, 2 - x line is kept

static char *read_file(const char *path, size_t *out_len)
{
	struct file *filp;
	char *buffer = NULL;
	loff_t pos = 0;
	loff_t file_size;
	ssize_t bytes_read;
	pr_info("Scan: Opening file for reading: %s\n", path);
	
	filp = filp_open(path, O_RDONLY, 0);
	if(IS_ERR(filp)){
		pr_err("Scan: Failed to open file: %s\n", path);
		return NULL;
	}
	file_size = i_size_read(file_inode(filp));
	
	if(file_size <= 0) {
		pr_err("Scan: The size of the file is too small: %s\n", path);
		filp_close(filp, NULL);
		return NULL;
	}
	if(file_size > 10 * 1024 * 1024) { //10 MB
		pr_err("Scan: The file size is too large (max 10MB): %s\n", path);
		filp_close(filp, NULL);
		return NULL;
	}

	buffer = vmalloc(file_size);
	if(!buffer){
		pr_err("Scan: Failed to allocate memory for the file: %s\n", path);
		filp_close(filp, NULL);
		return NULL;
	}

	bytes_read = kernel_read(filp, buffer, file_size, &pos);
	if(bytes_read != file_size) {
		pr_err("Scan: Mismatch between inode and kernel size reads of the file: %s\n", path);
		vfree(buffer); //remember to free the memory
		filp_close(filp, NULL);
		return NULL;
	}

	*out_len = file_size;
	filp_close(filp, NULL);

	pr_info("Scan: Closing the read file: %s\n", path);
	return buffer;
}

static int write_file(const char* path, const char *data, size_t len)
{
	struct file *filp;
	loff_t pos = 0;
	ssize_t bytes_wr;

	pr_info("Scan: Opening file for writing: %s\n", path);
	filp = filp_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
	
	if(IS_ERR(filp)) {
		pr_err("Scan: Failed to open file: %s\n", path);
		return PTR_ERR(filp);
	}

	bytes_wr = kernel_write(filp, data, len, &pos);
	filp_close(filp, NULL);
	
	if(bytes_wr != len) {
		pr_err("Scan: Not enough bytes to write to the file: %s\nThe required: %zu\nThe written: %zu", path, len, bytes_wr);
		return -EIO;
	}

	pr_info("Scan: Closed file for writing: %s wrote %zu bytes\n", path, bytes_wr);
	return 0;

}

size_t size_per(const char* input, size_t input_len)
{
	size_t i;
	size_t tot_lines = 0;
	size_t bytes_needed = 0;

	if(dsmp_line_per > 100 || dsmp_line_per < 0) {
		pr_err("Scan: The desampling percentage is incorrect. (RANGE 0 - 100)\n");
		return 0;
	}

	for(i = 0; i < input_len; i++) {
		if(input[i] == '\n')
			tot_lines++;
	}

	//checking the possible last line if it would not have \n at the end
	if(input[input_len-1] != '\n')
		tot_lines++;

	if(tot_lines <= 0){
		pr_err("Scan: The input file does not contain lines.\n");
		return 0;
	}

	size_t curr_line = 0;
	size_t bytes_so_far = 0;
	for(i = 0; i<input_len; i++) {
		if(input[i] == '\n') {
			size_t line_len = i - bytes_so_far + 1;

			if((curr_line * 100) >= (dsmp_line_per * tot_lines))
				bytes_needed += line_len;
			
			curr_line++;
			bytes_so_far = i + 1;
		}
	}

	if(bytes_so_far < input_len){ //some line left
		size_t line_len = input_len - bytes_so_far;

		if((curr_line * 100) >= (dsmp_line_per * tot_lines))
			bytes_needed += line_len;
	}

	return bytes_needed;
}

size_t size_line(const char* input, size_t input_len)
{
	size_t i;
	size_t tot_lines = 0;
	size_t bytes_needed = 0;

	for(i = 0; i < input_len; i++) {
		if(input[i] == '\n')
			tot_lines++;
	}

	//checking the possible last line if it would not have \n at the end
	if(input[input_len-1] != '\n')
		tot_lines++;

	if(tot_lines <= 0){
		pr_err("Scan: The input file does not contain lines.\n");
		return 0;
	}

	if(dsmp_line_num > tot_lines || dsmp_line_num <= 0) {
		pr_err("Scan: The desampling line number is incorrect. (RANGE 0 - %zu)\n", tot_lines);
		return 0;
	}

	size_t curr_line = 0;
	size_t bytes_so_far = 0;
	if(dsmp_mode == 1){ //1 - x line is ignored
		for(i = 0; i<input_len; i++) {
			if(input[i] == '\n') {
				size_t line_len = i - bytes_so_far + 1;

				if(curr_line % dsmp_line_num != 0)
					bytes_needed += line_len;
				
				curr_line++;
				bytes_so_far = i + 1;
			}
		}
	}else if(dsmp_mode == 2){ // 2 - x line is kept
		for(i = 0; i<input_len; i++) {
			if(input[i] == '\n') {
				size_t line_len = i - bytes_so_far + 1;

				if(curr_line % dsmp_line_num == 0)
					bytes_needed += line_len;
				
				curr_line++;
				bytes_so_far = i + 1;
			}
		}
	}else{
		pr_err("Scan: Incorrect desampling mode in finding size for x line method: %u mode value\n", dsmp_mode);
		return 0;
	}
	

	if(bytes_so_far < input_len){ //some line left
		size_t line_len = input_len - bytes_so_far;

		if(dsmp_mode == 1){

			if(curr_line % dsmp_line_num != 0)
				bytes_needed += line_len;

		}else if(dsmp_mode == 2){

			if(curr_line % dsmp_line_num == 0)
				bytes_needed += line_len;

		}else{ //you never know, maybe a bit paranoid
			pr_err("Scan: Incorrect desampling mode in finding size for x line method: %u mode value\n", dsmp_mode);
			return 0;
		}
		
	}

	return bytes_needed;
}

size_t find_size(const char* input, size_t input_len)
{
	pr_info("Scan: Finding the amount of bytes the output needs to allocate\n");
	size_t bytes_needed = 0;
	
	if(dsmp_mode == 0){

		bytes_needed = size_per(input,input_len);

	}else if(dsmp_mode == 1 || dsmp_mode == 2){

		bytes_needed = size_line(input,input_len);

	}else{

		pr_err("Scan: Incorrect desampling mode in find size method: %u\n", dsmp_mode);
		return 0;
	}
	
	pr_info("Scan: Amount of bytes:\nInput: %zu bytes\nOutput needs to allocate: %zu bytes\n", input_len, bytes_needed);	

	return bytes_needed;
}

static char* downsampling_per(char* output, const char* input, size_t input_len, size_t* output_len, size_t tot_lines){
	size_t curr_line = 0;
	size_t bytes_so_far = 0;
	size_t out_pos = 0;
	size_t i;
	for(i = 0; i<input_len; i++) {
		if(input[i] == '\n') {
			size_t line_len = i - bytes_so_far + 1;

			if((curr_line * 100) >= (dsmp_line_per * tot_lines)) {
				if(out_pos + line_len > *output_len) {
					pr_err("Scan: Buffer overflow detected!\n");
					vfree(output);
					return NULL;
				}

				memcpy(output + out_pos, input + bytes_so_far, line_len);
				out_pos += line_len;
			}
				
			
			curr_line++;
			bytes_so_far = i + 1;
		}
	}

	if(bytes_so_far < input_len){ //some line left
		
		if((curr_line * 100) >= (dsmp_line_per * tot_lines)) {

			size_t line_len = input_len - bytes_so_far;

				if(out_pos + line_len > *output_len) {
					pr_err("Scan: Buffer overflow detected!\n");
					vfree(output);
					return NULL;
				}

				memcpy(output + out_pos, input + bytes_so_far, line_len);
				out_pos += line_len;
			}
	}
	return output;
}

static char* downsampling_line(char* output, const char* input, size_t input_len, size_t* output_len, size_t tot_lines){
	size_t curr_line = 0;
	size_t bytes_so_far = 0;
	size_t out_pos = 0;
	size_t i;

	if(dsmp_line_num > tot_lines || dsmp_line_num <=0){
		pr_err("Scan: The desampling line number is incorrect. (RANGE 0 - %zu)\n", tot_lines);
		return NULL;
	}

	if(dsmp_mode == 1){
		for(i = 0; i<input_len; i++) {
			if(input[i] == '\n') {
				size_t line_len = i - bytes_so_far + 1;

				if(curr_line % dsmp_line_num != 0) {
					if(out_pos + line_len > *output_len) {
						pr_err("Scan: Buffer overflow detected!\n");
						vfree(output);
						return NULL;
					}

					memcpy(output + out_pos, input + bytes_so_far, line_len);
					out_pos += line_len;
				}
					
				
				curr_line++;
				bytes_so_far = i + 1;
			}
		}
	}else if(dsmp_mode == 2){
		for(i = 0; i<input_len; i++) {
			if(input[i] == '\n') {
				size_t line_len = i - bytes_so_far + 1;

				if(curr_line % dsmp_line_num == 0) {
					if(out_pos + line_len > *output_len) {
						pr_err("Scan: Buffer overflow detected!\n");
						vfree(output);
						return NULL;
					}

					memcpy(output + out_pos, input + bytes_so_far, line_len);
					out_pos += line_len;
				}
					
				
				curr_line++;
				bytes_so_far = i + 1;
			}
		}
	}else{
		pr_err("Scan: Incorrect desampling mode in desampling line method: %u mode\n", dsmp_mode);
		return NULL;
	}
	
	if(bytes_so_far < input_len){ //some line left
		if(dsmp_mode == 1){

			if(curr_line % dsmp_line_num != 0) {

				size_t line_len = input_len - bytes_so_far;

					if(out_pos + line_len > *output_len) {
						pr_err("Scan: Buffer overflow detected!\n");
						vfree(output);
						return NULL;
					}

					memcpy(output + out_pos, input + bytes_so_far, line_len);
					out_pos += line_len;
				}
		}else if(dsmp_mode == 2){
			if(curr_line % dsmp_line_num == 0) {

				size_t line_len = input_len - bytes_so_far;

					if(out_pos + line_len > *output_len) {
						pr_err("Scan: Buffer overflow detected!\n");
						vfree(output);
						return NULL;
					}

					memcpy(output + out_pos, input + bytes_so_far, line_len);
					out_pos += line_len;
				}
		}else{
			pr_err("Scan: Incorrect desampling mode in desampling line method: %u mode\n", dsmp_mode);
			return NULL;
		}
		
	}
	return output;
}

static char* downsampling(const char* input, size_t input_len, size_t* output_len)
{
	char* output;
	size_t tot_lines = 0;
	size_t i;

	*output_len = find_size(input,input_len);

	if(*output_len <= 0){
		pr_err("Scan: The output length cannot be %zu\n", *output_len);
		return NULL;
	}

	for(i = 0; i < input_len; i++) {
		if(input[i] == '\n')
			tot_lines++;
	}

	//checking the possible last line if it would not have \n at the end
	if(input[input_len-1] != '\n')
		tot_lines++;

	if(tot_lines <= 0){
		pr_err("Scan: The input file does not contain lines.\n");
		return 0;
	}

	output = vmalloc(*output_len);
	if(!output) {
		pr_err("Scan: Failed to allocate %zu bytes\n", output_len);
		return NULL;
	}

	// divide for algorithms
	if(dsmp_mode == 0){
		output = downsampling_per(output, input, input_len, output_len, tot_lines);
	}else if(dsmp_mode == 1 || dsmp_mode == 2){
		output = downsampling_line(output, input, input_len, output_len, tot_lines);
	}else{
		pr_err("Scan: Incorrect desampling mode in desampling method: %u mode\n", dsmp_mode);
		return NULL;
	}
	pr_info("Scan: Finished downsampling algorithm, mode: %u\n", dsmp_mode);

	return output;
}

static int process_files(void)
{
	char *i_data = NULL;
	char *fltr_data = NULL;
	size_t i_len, fltr_len;
	size_t out_len;
	int ret = 0;
	pr_info("Scan: Processing files\n");

	if(input_path[0] == '\0') {
		pr_err("Scan: The path to the input file is empty\n");
		return -EINVAL;
	}

	if(output_path[0] == '\0') {
		pr_err("Scan: The path to the output file is empty\n");
		return -EINVAL;
	}

	i_data = read_file(input_path, &i_len);
	if(!i_data) {
		pr_err("Scan: Could not read the input data from: %s\n", input_path);
		return -EIO;
	}
	pr_info("Scan: Read the input file: %s %zu bytes\n", input_path, i_len);

	fltr_data = downsampling(i_data, i_len, &out_len);

	
	ret = write_file(output_path, fltr_data, out_len); //if out_len is 0 then it will still proceed on purpose.
	if(ret < 0) {
		pr_err("Scan: Failed to write to the output file: %s\n", output_path);
		goto cleanup_all;
	}

	pr_info("Scan: Written to the output: %s %zu bytes\n", output_path, out_len);

cleanup_all:
	vfree(fltr_data);
cleanup_i:
	vfree(i_data);

	return ret;
}

static int scan_open(struct inode *inode, struct file *filp)
{
	pr_info("Scan: Scan miscdevice open\n");
	return 0;
}


static ssize_t scan_read(struct file *filp, char __user *buf, size_t count,
	loff_t *off)
{
	pr_info("Scan: Scan miscdevice on read\n");
	ssize_t ret = 0;
	size_t status_len;

	mutex_lock(&scan_lock);
	
	status_len = strlen(status_msg);	
	if(*off >= status_len){
		pr_err("Scan: The file reading offset does not match the length of the message. Possible mutex error\n");
		ret = 0;
		goto out_unlock;
	}
	if(count >= status_len - *off)
		count = status_len - *off;
	
	ret = -EFAULT; //by default if we fail, there is an error
	if(copy_to_user(buf, status_msg + *off, count)){ //if returns > 0 then there's an error 
		pr_err("Scan: Could not copy the %zu bytes of data to the user buffer from the miscdevice scan\n", count);
		goto out_unlock;
	}
	ret = count; //complete success

	*off += ret;
	pr_info("Scan: Scan miscdevice successfully read\n");

out_unlock:
	mutex_unlock(&scan_lock);
	return ret;
}

static ssize_t scan_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *off)
{
	pr_info("Scan: Scan miscdevice on write\n");
	char *kbuf;
	ssize_t ret = count;

	if(count == 0 || count > MAX_PATH_LEN + 10){
		pr_err("Scan: The amount of data written is incorrect\n");
		return -EINVAL;
	}

	kbuf = kmalloc(count + 1, GFP_KERNEL);
	if(!kbuf){
		pr_err("Scan: Could not allocate the kernel buffer\n");
		return -ENOMEM;
	}

	if(copy_from_user(kbuf, buf, count)) {
		pr_err("Scan: Error with copying data from the kernel buffer to the user buffer\n");
		kfree(kbuf);
		return -EFAULT;
	}
	kbuf[count] = '\0';

	if(count > 0 && kbuf[count - 1] == '\n')
		kbuf[count - 1] = '\0';

	mutex_lock(&scan_lock);

	if(strncmp(kbuf, "INPUT:", 6) == 0) {

		strncpy(input_path, kbuf + 6, MAX_PATH_LEN - 1);
		input_path[MAX_PATH_LEN - 1] = '\0';
		pr_info("Scan: Saved input path: %s\n", input_path);

	} else if(strncmp(kbuf, "OUTPUT:", 7) == 0) {

		strncpy(output_path, kbuf + 7, MAX_PATH_LEN - 1);
		output_path[MAX_PATH_LEN - 1] = '\0';
		pr_info("Scan: Saved output path: %s\n", output_path);

	} else if(strncmp(kbuf, "PER:", 4) == 0) {

		unsigned int buff_per;
		if(kstrtouint(kbuf + 4, 10, &buff_per) == 0){
			if(buff_per <= 100 && buff_per >= 0){
				dsmp_line_per = buff_per;
				pr_info("Scan: Saved percentage of filtered out lines: %u / 100\n", dsmp_line_per);
			}else{
				pr_err("Scan: Incorrect percentage, range 0 - 100\n");
				ret = -EINVAL;
			}
		} else {
			pr_err("Scan: Incorrect percentage, range 0 - 100\n");
			ret = -EINVAL;
		}
		
	} else if(strncmp(kbuf, "LINE:", 5) == 0) {
		unsigned int buff_line;
		if(kstrtouint(kbuf + 5, 10, &buff_line) == 0){
			if(buff_line > 0){
				dsmp_line_num = buff_line;
				pr_info("Scan: Saved number of lines per filtering: %u\n", dsmp_line_num);
			}else{
				pr_err("Scan: Incorrect number of lines, range from 1\n");
				ret = -EINVAL;
			}
		} else {
			pr_err("Scan: Incorrect number of lines, range from 1\n");
			ret = -EINVAL;
		}
	} else if(strncmp(kbuf, "MODE:", 5) == 0) {
		unsigned int buff_mode;
		if(kstrtouint(kbuf + 5, 10, &buff_mode) == 0){
			if(buff_mode >= 0 && buff_mode < 3){
				dsmp_mode = buff_mode;
				pr_info("Scan: Saved mode: %u\n", dsmp_mode);
			}else{
				pr_err("Scan: Incorrect downsampling mode, correct values:\n0 - percentage from 0 to 100\n1 - x line is ignored\n2 - x line is kept\nIncorrect given value: %u\n", buff_mode);
				ret = -EINVAL;
			}
		} else {
			pr_err("Scan: Incorrect downsampling mode, correct values:\n0 - percentage from 0 to 100\n1 - x line is ignored\n2 - x line is kept\nIncorrect given value: %u\n", buff_mode);
			ret = -EINVAL;
		}
	} else if(strncmp(kbuf, "MAN", 3) == 0) {
		pr_info("Scan: Man\nScan kernel module made by Jakub Gomola.\nThis module takes data from the input path,\nUses downsampling algorithm (3 available)\nAnd saves the filtered out data to the output path.\nAvailable commands:\nMAN - manual\nINFO - information about saved variables and options\nINPUT:path - saves the input path (necessary)\nOUTPUT:path - saves the output path (necessary, CAUTION, you are responsible for the path you provide.)\nMODE:[0-2] - desampling algorithm of your choice:\n		0 - percentage of input data lines is filtered out\n 	1 - each x line is filtered out\n 	2 - each x line is kept\nPER:[0-100] - the percentage value for MODE = 0\nLINE:[1-number of lines in the input file] - the x line value for MODE = 1 or MODE = 2\nRUN - runs the algorithm\n");
	} else if(strncmp(kbuf, "INFO", 4) == 0) {
		pr_info("Scan: Information\nMode: %u\nInput: %s\nOutput: %s\nDesampling procent: %u\nDesampling line: %u\nLast running status: %u\n", dsmp_mode, input_path, output_path, dsmp_line_per, dsmp_line_num, last_status);
	} else if(strcmp(kbuf, "RUN") == 0) {

		pr_info("Scan: Running the scan module");
		last_status = process_files();

	} else {

		pr_err("Scan: Unknown command for the scan module\nAvailable are:\nINPUT:path\nOUTPUT:path\nMODE:[0-2]\nPER:[0-100]\nLINE:[1-number of lines in input file]\nINFO\nRUN.\n");
		ret = -EINVAL;

	}

	mutex_unlock(&scan_lock);
	kfree(kbuf);
	return ret;
}

static int scan_release(struct inode *inode, struct file *filp)
{
	pr_info("Scan: Scan miscdevice release\n");
	return 0;
}

static const struct file_operations scan_fops = {
	.owner = THIS_MODULE,
	.open = scan_open,
	.read = scan_read,
	.write = scan_write,
	.release = scan_release,
};

static struct miscdevice scan_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scan",
	.fops = &scan_fops,
	.mode  = 0666,
};

static int __init scan_init(void)
{
	pr_info("Scan: Initiating the scan module\n");

	mutex_init(&scan_lock);

	input_path[0] = '\0';
	output_path[0] = '\0';

	snprintf(status_msg, sizeof(status_msg), "Ready");
	last_status = 0;
	scan_len = 0;

	int ret;

	ret = misc_register(&scan_miscdevice);
	if(ret < 0){
		pr_err("Scan: Cannot register miscdevice scan.\n");
		return ret;
	}
	pr_info("Scan: Registered miscdevice with minor %d\n", scan_miscdevice.minor);
	return 0;
}

static void __exit scan_exit(void)
{
	misc_deregister(&scan_miscdevice);
	mutex_destroy(&scan_lock);
	pr_info("Scan: Unloading the scan module\n");
}

module_init(scan_init);
module_exit(scan_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Gomola");
MODULE_DESCRIPTION("Scan Data Kernel Module");
MODULE_VERSION("1.6");