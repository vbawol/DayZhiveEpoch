import os
import sys

with os.popen("git rev-list HEAD", "r") as out:
	version = out.read().split("\n")[0].strip("\r")
	with open(sys.argv[1], "rb") as _in:
		with open(sys.argv[2], "wb") as _out:
			_out.write(_in.read().replace("%GIT_VERSION%", version))