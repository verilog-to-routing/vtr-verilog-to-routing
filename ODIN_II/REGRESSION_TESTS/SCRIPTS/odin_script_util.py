#! /usr/bin/python

import re
import string

def isType(filename, filetype):
	"""Return true if the filename as the filetype extension"""
	return re.search("\." + filetype + "$", filename) != None

def isBlif(file): 
	"""Return true if the file has the .blif extension."""
	return re.search("\.blif$", file) != None
	
def isBlifMV(file): 
	"""Return true if the file has the .mv extension."""
	return re.search("\.mv$", file) != None

def isVerilog(file): 
	"""Return true if the file has the .v extension."""
	return re.search("\.v$", file) != None
	
def isXML(file): 
	"""Return true if the file has the .xml extension."""
	return re.search("\.xml$", file) != None

def isSoft(file): 
	return re.search("soft\.blif$", file) != None

def trimExtensions(filename):
	"""return the string up until the first period."""
	return filename[0:string.find(filename, "."):1]
	
def trimDotV(filename):
	"""Trim the .v from a file name entry."""
	return filename[0:string.find(filename, ".v"):1]
	