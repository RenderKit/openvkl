## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

import subprocess
import os
import platform
import sys
import signal
from threading import Timer

# Flag for enabling additional verbose debug info
debug_enabled = True

# Representation of output which goes to console.
# It consist of stdout & stderr.
class Output(object):
    stdout: str
    stderr: str
    def __init__(self, stdout:str, stderr:str):
        self.stdout = stdout
        self.stderr = stderr

    # By default we can get string from instance of this class
    # which will return merged stdout with stderr.
    def __str__(self):
        return self.stdout + self.stderr

class TestCommandTool:
    def run(self, cmd, timeout, cwd = os.getcwd(), test_env = os.environ.copy(), print_output = True):
        exit_code = 0
        proc = subprocess.Popen(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=None, universal_newlines=True, env=test_env)

        # This variable we use to pass value back from
        # Timer callback function - _kill.
        timeout_flag_wrapper = {'timeout_occured': False}

        # We use timer to not hang forever on
        # proc.communicate(), in case of timeout
        # _kill method will kill process and
        # it will return from proc.communicate()
        # immediately.
        timer = Timer(timeout, self._kill, [proc, timeout_flag_wrapper], {})
        timer.start()
        stdout, stderr = proc.communicate()
        timer.cancel()
        exit_code = proc.poll()

        if timeout_flag_wrapper["timeout_occured"]:
            exit_code = 124
            stderr += "=K=> Timeout expired, process was killed."

        output = Output(stdout, stderr)
        if print_output:
            print(output, flush=True)

        return (exit_code, output)

    def _kill(self, proc_to_kill, timeout_flag_wrapper):
        timeout_flag_wrapper["timeout_occured"] = True
        pid = proc_to_kill.pid
        # Windows
        if platform.system() == 'Windows':
            proc = subprocess.Popen(['taskkill', '/F', '/T', '/PID', str(pid)], shell=True)
            proc.wait()
        # Linux
        else:
            proc = subprocess.Popen('pkill -TERM -P '+ str(pid), shell=True)
            proc.wait()

class OpenVKLTestCase:
    renderer:str = None
    volume_type:str = None
    max_mse:float = None
    spp:int = None

    __gpu_exit_code:int = None
    __gpu_output:Output = None

    __cpu_exit_code:int = None
    __cpu_output:Output = None

    __diff_exit_code:int = None
    __diff_output:Output = None

    def __init__(self, renderer : str, volume_type : str):
        self.renderer = renderer
        self.volume_type = volume_type
        self.max_mse = 0.000001
        self.spp = 2

        # For this particular case we need to set higher MSE treshold
        if renderer == "hit_iterator_renderer" and volume_type == "structuredRegular":
            self.max_mse = 0.000015

        # For density pathtracer we want more spp to get picutre closer to final image
        if renderer == "density_pathtracer":
            self.spp = 50

    def __print_debug(self, msg:str):
        if debug_enabled:
            print(msg)

    def __get_example_cpu_binary_string(self) -> str:
        if platform.system() == 'Windows':
            return "vklExamplesCPU.exe"
        else:
            return "./vklExamplesCPU"

    def __get_example_gpu_binary_string(self) -> str:
        if platform.system() == 'Windows':
            return "vklExamplesGPU.exe"
        else:
            return "./vklExamplesGPU"

    def __get_common_params(self) -> str:
        return "-batch -framebufferSize 1024 1024"

    def get_name(self) -> str:
        return "%s-%s" % (self.renderer, self.volume_type)

    def get_result(self) -> int:
        return self.__gpu_exit_code + self.__cpu_exit_code + self.__diff_exit_code

    def print_error_outputs(self):
        # Print output only for commands where exit code != 0
        if self.__gpu_exit_code != 0:
            print("#!# GPU cmd output:")
            print(self.__gpu_output)

        if self.__cpu_exit_code != 0:
            print("#!# CPU cmd output:")
            print(self.__cpu_output)

        if self.__diff_exit_code != 0:
            print("#!# DIFF cmd output:")
            print(self.__diff_output)

    def get_MSE(self) -> float:
        stdout = self.__diff_output.stdout
        # MSE can't be negative and this is how we're returning error
        if (len(stdout) == 0) or (":" not in stdout):
            return -1.0
        return float(stdout.splitlines()[0].split(": ")[1])

    def execute(self, img_diff_tool_path:str):
        # Default timeout - 60 secs for each command
        timeout = 60

        # Execute GPU example
        gpu_run_cmd = "%s -renderer %s_gpu %s -volumeType %s -spp %d" % (self.__get_example_gpu_binary_string(), self.renderer, self.__get_common_params(), self.volume_type, self.spp)
        self.__print_debug("## Executing: '%s', with timeout: %d" % (gpu_run_cmd, timeout))
        self.__gpu_exit_code, self.__gpu_output = TestCommandTool().run(gpu_run_cmd, timeout)

        # Execute CPU example
        cpu_run_cmd = "%s -renderer %s %s -volumeType %s -spp %d" % (self.__get_example_cpu_binary_string(), self.renderer, self.__get_common_params(), self.volume_type, self.spp)
        self.__print_debug("## Executing: '%s', with timeout: %d" % (cpu_run_cmd, timeout))
        self.__cpu_exit_code, self.__cpu_output = TestCommandTool().run(cpu_run_cmd, timeout)

        # Rename generated images to new name pattern "renderer-volume_type" instead of "renderer"
        # so all images can be stored in the same directory. That way we can avoid overriding output image
        # by different volume types executions.
        src_gpu_file_path = os.path.join(os.getcwd(), "%s_gpu.pfm" % self.renderer)
        dst_gpu_file_path = os.path.join(os.getcwd(), "%s-%s-gpu.pfm" % (self.renderer, self.volume_type))
        os.rename(src_gpu_file_path, dst_gpu_file_path)

        src_cpu_file_path = os.path.join(os.getcwd(), "%s.pfm" % self.renderer)
        dst_cpu_file_path = os.path.join(os.getcwd(), "%s-%s-cpu.pfm" % (self.renderer, self.volume_type))
        os.rename(src_cpu_file_path, dst_cpu_file_path)

        # Calculate difference between GPU & CPU generated image
        img_diff_cmd = "%s %s %s %.10f" % (img_diff_tool_path, dst_gpu_file_path, dst_cpu_file_path, self.max_mse)
        self.__print_debug("## Executing: '%s', with timeout: %d" % (img_diff_cmd, timeout))
        self.__diff_exit_code, self.__diff_output = TestCommandTool().run(img_diff_cmd, timeout)

        self.__print_debug("## MSE: %.10f" % self.get_MSE())
        self.__print_debug("## Exit codes: %d %d %d" % (self.__gpu_exit_code, self.__cpu_exit_code, self.__diff_exit_code))

def main():
    if len(sys.argv) <= 1:
        print("#!## [ERROR] First argument must contain path to diff_tool");
        return 2

    img_diff_tool_path = sys.argv[1]
    test_cases = []

    # generate test cases
    renderer_list = ["density_pathtracer", "ray_march_iterator", "interval_iterator_debug", "hit_iterator_renderer"]
    volume_type_list = ["structuredRegular", "structuredSpherical", "unstructured", "particle", "amr", "vdb"]
    for renderer in renderer_list:
        for volume_type in volume_type_list:
            test_cases.append(OpenVKLTestCase(renderer, volume_type))

    # execute test cases
    for test_case in test_cases:
        test_case.execute(img_diff_tool_path)

    # print summary & analyze results
    print()
    print("################################ SUMMARY ################################")
    print()

    failed_test_cases = []
    # For any more advanced table formatting external library should be used
    # or external class should be created.
    fixed_width_row_format = "%-5s %-7s %-45s %s"
    print(fixed_width_row_format % ("####", "Result", "Test case name", "MSE value"))
    print("------------------------------------------------------------------------")
    for test_case in test_cases:
        result = test_case.get_result()
        if result != 0:
            log_prefix = "#!##"
            result_str = "[FAIL]"
            failed_test_cases.append(test_case)
        else:
            log_prefix = "####"
            result_str = "[PASS]"
        print(fixed_width_row_format % (log_prefix, result_str, test_case.get_name(), "%.10f" % test_case.get_MSE()))

    total_count = len(test_cases)
    fail_count = len(failed_test_cases)
    pass_count = total_count - fail_count

    print()
    if fail_count == 0:
        print("#### All test cases PASSED (passrate: %d/%d)" % (pass_count, total_count))
        return 0

    print("#!## Some test cases FAILED (passrate: %d/%d)" % (pass_count, total_count))
    print()
    # Print output from failed tests
    for test_case in failed_test_cases:
        print("#!## '%s' failure details:" % test_case.get_name())
        test_case.print_error_outputs()
    return 1

if __name__ == "__main__":
    sys.exit(main())