#!/bin/python
# Updates /src/enum/icons after all icons in res.qrc
import pathlib
import xml.sax

values: list[str] = []


class ResHandler(xml.sax.handler.ContentHandler):
	def characters(self, content):
		if not str(content).startswith("res/ic/dark"):
			return
		path = pathlib.Path(content)
		name = path.stem[0].upper()
		for i, char in enumerate(path.stem[1:]):
			if char != "-":
				name += str(char).upper() if path.stem[i] == "-" else char
		values.append(f"\t{name},\n")


reader = xml.sax.make_parser()
reader.setContentHandler(ResHandler())
reader.parse(pathlib.Path.cwd().parent.joinpath("res.qrc"))

with open("../src/enum/icons.hpp", "w") as icons:
	icons.writelines([
		"#pragma once\n\n",
		"// This file is automatically generated from scripts/update/icons_enum.py\n",
		"// Do not modify!\n\n",
		"enum class Icons: char\n",
		"{\n",
		*values,
		"};\n\n",
	])
