extends Object

var GDXY2 = preload("res://bin/gdxy2.gdns")
var gdxy2 = GDXY2.new()

var path: String
var offset: int
var size: int
var flag: String
var head_size: int
var direction_num: int
var frame_num: int
var pic_num: int
var width: int
var height: int
var x: int
var y: int
var pal: PoolByteArray
var pic_offsets: PoolIntArray
var frames: Array
var time: PoolIntArray

func _init(path: String, offset: int = 0, size: int = 0):
	if path == null: return
	self.path = path
	self.offset = offset

	var file = File.new()
	var err = file.open(path, File.READ)
	if err: 
		push_error("File Not Existed")
		return
		
	if size == 0:
		file.seek_end()
		self.size = file.get_position()
	else:
		self.size = size
	file.seek(offset)
	
	self.flag = file.get_buffer(2).get_string_from_utf8()
	self.head_size = file.get_16()
	
	self.direction_num = file.get_16()
	self.frame_num = file.get_16()
	self.pic_num = self.direction_num * self.frame_num
	self.width = file.get_16()
	self.height = file.get_16()
	self.x = file.get_16()
	self.y = file.get_16()
	
	if self.head_size > 12:
		for i in range(self.head_size - 12):
			self.time.append(file.get_8())
			
	self.pal = gdxy2.format_pal(file.get_buffer(512))
	
	for i in range(self.pic_num):
		self.pic_offsets.append(file.get_32() + self.offset + 4 + self.head_size)
		
	self.frames = []
	self.frames.resize(self.direction_num)
	for i in range(self.direction_num):
		self.frames[i] = []
		self.frames[i].resize(self.frame_num)
		for j in range(self.frame_num):
			var index = i * self.direction_num + j
			file.seek(self.pic_offsets[index])
			var frame = {}
			frame.x = file.get_32()
			frame.y = file.get_32()
			frame.width = file.get_32()
			frame.height = file.get_32()
			
			var frame_size: int
			if index < self.pic_num - 1:
				frame_size = self.pic_offsets[index + 1] - self.pic_offsets[index]
			else:
				frame_size = self.size - self.pic_offsets[index]
			
			file.seek(self.pic_offsets[index])
			var buff = file.get_buffer(frame_size + 16)
			var pba = gdxy2.read_was(buff, self.pal)
			
			frame.img = Image.new()
			frame.img.create_from_data(frame.width, frame.height, false, Image.FORMAT_RGBA8, pba)
			
			self.frames[i][j] = frame

	file.close()
	
	
func getFrame(d: int, f: int):
	return self.frames[d][f]

