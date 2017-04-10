import os
import sys

with os.popen("git rev-list HEAD ^origin", "r") as out:
	version = out.read().split("\n")[0].strip("\r")
	with open(sys.argv[1], "r") as _in:
		with open(sys.argv[2], "w") as _out:
			_out.write(_in.read().replace("%GIT_VERSION%", version))