import time
import logging
import collections
from ... import version
import os
import json

log = logging.getLogger("gramtools")


def with_report(f):
    """
    Decorator to add logging and reporting to build procedures
    To signal that something went wrong in decorated function call, that function needs to raise an Exception.
    """

    def reportify(report, action, *args):
        if report.get("success") is False:
            report[action] = {"success": False}
            return report

        success, error, command, stdout = True, None, None, None
        timer_start = time.time()

        try:
            original_result = f(report, action, *args)
        except Exception as e:
            success = False
            error = str(e)
            original_result = None
        timer_end = time.time()

        log.debug(f"Ran {action} in: {timer_end - timer_start} seconds")

        report["success"] = success
        action_report = collections.OrderedDict(
            [
                ("success", success),
                ("error_message", error),
                ("Run time", int(timer_end) - int(timer_start)),
            ]
        )

        # The condition below allows the called function to modify the report as well.
        if action not in report:
            report[action] = action_report
        else:
            report[action].update(action_report)

        return original_result

    return reportify


def _save_report(
    start_time, report, command_paths, command_hash_paths, report_file_path
):
    end_time = str(time.time()).split(".")[0]
    _, report_dict = version.report()
    current_working_directory = os.getcwd()

    _report = collections.OrderedDict(
        [
            ("start_time", start_time),
            ("end_time", end_time),
            ("total_runtime", int(end_time) - int(start_time)),
        ]
    )
    _report.update(report)
    _report.update(
        collections.OrderedDict(
            [
                ("current_working_directory", current_working_directory),
                ("paths", command_paths),
                ("path_hashes", command_hash_paths),
                ("version_report", report_dict),
            ]
        )
    )

    with open(report_file_path, "w") as fhandle:
        json.dump(_report, fhandle, indent=4)
