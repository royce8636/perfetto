# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import subprocess
from os.path import relpath


def CheckChange(input, output):
    results = []
    results += CheckTslint(input, output)
    return results


def CheckChangeOnUpload(input_api, output_api):
    return CheckChange(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
    return CheckChange(input_api, output_api)


def CheckTslint(input_api, output_api):
    path = input_api.os_path
    ui_path = input_api.PresubmitLocalPath();
    node = path.join(ui_path, 'node')
    tslint = path.join(ui_path, 'node_modules', '.bin', 'tslint')

    # Some tslint rules require type information and thus need the whole
    # project. We therefore call tslint on the whole project instead of only the
    # changed files. It is possible to break tslint on files that was not
    # changed by changing the type of an object.
    if subprocess.call([node, tslint, '--project', ui_path,
                        '--format', 'codeFrame']):
        return [
            output_api.PresubmitError("""\
There were tslint errors. You may be able to fix some of them using
$ {} {} --project {} --fix""".format(relpath(node), relpath(tslint), ui_path))
        ]
    return []
