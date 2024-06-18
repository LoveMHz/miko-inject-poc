#!/usr/bin/env python
import sys
import ctypes
import pefile

from external.pyxbe.xbe import Xbe, XbeSection, XbeSectionHeader

# Load in retail Zero executable
xbe = Xbe.from_file('resources/default.xbe')

# Load in our EXE for injection
pe = pefile.PE("main.exe")

#
disallowed_sections = ['I$$.edataxb']

# Attempt to find ''
peImportTableOffset = False
for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
	if exp.name.decode('utf-8') == "ImportTableOffset":
		peImportTableOffset = exp.address
		print("Found peImportTableOffset", hex(peImportTableOffset))

# Inject all the sections
for section in pe.sections:
	# Mark injection sections with the 'I$$' prefix.
	section_name = "I$$" + section.Name.decode().rstrip('\x00')
	print(section_name)

	# Skip disallowed sections
	if section_name in disallowed_sections:
		continue

	header = XbeSectionHeader()
	header.flags        = XbeSectionHeader.FLAG_WRITABLE | XbeSectionHeader.FLAG_PRELOAD | XbeSectionHeader.FLAG_EXECUTABLE
	header.virtual_addr = 0x00400000 + section.VirtualAddress
	header.virtual_size = section.SizeOfRawData     # Yes... This is correct?
	header.raw_size     = section.Misc_VirtualSize  # Yes... This is correct?
	print("section.SizeOfRawData", section.SizeOfRawData)

	# Store local copy of section data
	section_data = section.get_data()

	# Pad section to raw_size??
	if len(section_data) < header.raw_size:
		section_data += bytes(b"\0" * (header.raw_size - len(section_data)))

	# Check if this section has 'ImportTableOffset'
	section_start = section.get_VirtualAddress_adj()
	section_end   = section_start + section.SizeOfRawData
	if peImportTableOffset and (peImportTableOffset >= section_start) and (peImportTableOffset < section_end):
		print("f peImportTableOffset", section_name)
		#section_data[0:4] = 0
		b = bytearray(section_data)
		iat_address = 0x00400000 + pe.OPTIONAL_HEADER.DATA_DIRECTORY[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IAT"]].VirtualAddress
		b[0:4] = (iat_address).to_bytes(4, 'little')
		section_data = bytes(b)

	#print("section offset", hex(section.get_VirtualAddress_adj()))
	#print(header)
	#print(section_data)

	# Add section to XBE
	xbe.sections[section_name] = XbeSection(section_name, header, section_data)

# Correct XBE total image size
last_section = sorted(xbe.sections, key=lambda x: xbe.sections[x].header.virtual_addr)[-1]
xbe.header.image_size = (xbe.sections[last_section].header.virtual_addr + xbe.sections[last_section].header.virtual_size - xbe.header.base_addr)

## Patch in entry point
xbe.header.entry_addr = (0x00400000 + pe.OPTIONAL_HEADER.AddressOfEntryPoint) ^ Xbe.ENTRY_RETAIL

for section in xbe.sections:
	a = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ]
	xbe.sections[section].header.digest = (ctypes.c_ubyte * len(a))(*a)

# Save XBE
xbe.pack()
