extends Object

var GDXY = preload("res://bin/gdxy.gdns")
var gdxy = GDXY.new()

var WAS = load("was.gd")

var path: String
var flag: String
var n: int
var file_dict: Dictionary

func _init(path):
	if path == null: return
	self.path = path

	var file = File.new()
	var err = file.open(path, File.READ)
	if err: 
		push_error ( "File Not Existed" ) 
		return
	
	self.flag = file.get_buffer(4).get_string_from_utf8()
	if self.flag != "PFDW":
		push_error ( "Not Valid WDF File" )
		return
	self.n = file.get_32()
	var offset = file.get_32()
	
	file.seek(offset)
	for i in range(self.n):
		var _hash: int = file.get_32()
		var _offset: int = file.get_32()
		var _size: int = file.get_32()
		var _spaces: int = file.get_32()
		var wdf_file = {"hash": _hash, "offset": _offset, "size": _size, "spaces": _spaces}
		self.file_dict[_hash] = wdf_file
	
	file.close()


func get(s: String):
	var _hash: int
	if s.begins_with("0x"):
		_hash = s.hex_to_int()
	else:
		_hash = gdxy.string_id(s)

	var file_info = self.file_dict[_hash]
	var file = File.new()
	file.open(path, File.READ)
	file.seek(file_info.offset)
	
	var flag: String = file.get_buffer(2).get_string_from_utf8()
	var item
	if flag == "SP":
		item = WAS.new(self.path, file_info.offset, file_info.size)
		
	file.close()
	return item
