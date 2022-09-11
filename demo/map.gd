extends Object

var GDXY = load("res://bin/gdxy.gdns")
var gdxy = GDXY.new()

const BLOCK_WIDTH = 320.0 #地图块宽度
const BLOCK_HEIGHT = 240.0 #地图块高度

var path         #文件路径
var flag
var width: int
var height: int
var rowNum: int
var colNum: int
var blockNum: int

var cellRowNum
var cellColNum
var astar = AStar2D.new()

var maskOffset: Array   #遮罩索引
var masks: Dictionary = {}
var no_repeat = {}

var blockOffset  #地图块偏移信息
var blocks = {}

var jpeg_head

var image        #图片

func _init(path):
	if path == null: return
	self.path = path

	var file = File.new()
	file.open(path, File.READ)
	
	self.flag = file.get_buffer(4).get_string_from_utf8()
	self.width = file.get_32()
	self.height = file.get_32()
	self.rowNum = ceil(self.height / BLOCK_HEIGHT)
	self.colNum = ceil(self.width / BLOCK_WIDTH)
	self.blockNum = self.rowNum * self.colNum
	
	self.cellRowNum = height / 20
	self.cellRowNum = self.cellRowNum if self.cellRowNum % 12 == 0 else self.cellRowNum + 12 - self.cellRowNum % 12
	self.cellColNum = width / 20
	self.cellColNum = self.cellColNum if self.cellColNum % 16 == 0 else self.cellColNum + 16 - self.cellColNum % 16
	
	self.blockOffset = []
	self.blockOffset.resize(self.blockNum)
	self.blockOffset.fill(0)
	for i in range(self.blockNum):
		self.blockOffset[i] = file.get_32()
		self.blocks[i] = {"ownMasks": []}
	file.get_32()  # 跳过无用的4字节 旧地图为MapSize  新地图为MASK Flag
	
	if self.flag == "0.1M":
		var maskNum = file.get_32()
		self.maskOffset = []
		self.maskOffset.resize(maskNum)
		self.maskOffset.fill(0)
		for i in range(self.maskOffset.size()):
			self.maskOffset[i] = file.get_32()
		for i in range(self.maskOffset.size()): 
			var offset = self.maskOffset[i]
			file.seek(offset)
			var maskInfo = {"id": i, "offset": offset + 20}
			maskInfo.x = file.get_32()
			maskInfo.y = file.get_32()
			maskInfo.width = file.get_32()
			maskInfo.height = file.get_32()
			maskInfo.size = file.get_32()
			var maskRowStart = max(maskInfo.y / BLOCK_HEIGHT, 0);
			var maskRowEnd = min((maskInfo.y + maskInfo.height) / BLOCK_HEIGHT, self.rowNum - 1)
			var maskColStart = max(maskInfo.x / BLOCK_WIDTH, 0);
			var maskColEnd = min((maskInfo.x + maskInfo.width) / BLOCK_WIDTH, self.colNum - 1);
			for x in range(maskRowStart, maskRowEnd + 1):
				for y in range(maskColStart, maskColEnd + 1):
					var index: int = x * self.colNum + y
					if 0 <= index and index < self.blockNum:
						self.blocks[index].ownMasks.append(i)
			self.masks[i] = maskInfo
	
	self.travel(file)
	file.close()
	
func travel(file):
	for i in range(self.blockNum):
		var offset = self.blockOffset[i]
		file.seek(offset)
		var eatNum = file.get_32()
		offset += 4
		if self.flag == "0.1M":
			for ignore in range(eatNum):
				file.get_32()
				offset += 4
		var loop = true
		while loop:
			file.seek(offset)
			var flag = file.get_buffer(4).get_string_from_utf8()
			var size = file.get_32()
			offset += 8
			
			if flag == "GEPJ" or flag == "2GPJ":
				self.blocks[i].jpegOffset = file.get_position()
				self.blocks[i].jpegSize = size
				offset += size
			elif flag == "KSAM" or flag == "2SAM":
				self.readOldMask(file, offset, i, size)
				offset += size
			elif flag == "LLEC":
				self.readCell(file, offset, i, size)
				offset += size
			elif flag == "GIRB":
				offset += size
			elif flag == "BLOK":
				offset += size
			else:
				loop = false
				
func readOldMask(file: File, offset: int, blockIndex: int, size: int):
	file.seek(offset)
	var maskInfo = {"id": 0, "offset": offset + 16}
	var row = blockIndex / self.colNum
	var col = blockIndex % self.colNum
	
	maskInfo.x = file.get_32()
	maskInfo.x = (col * 320) + maskInfo.x
	maskInfo.y = file.get_32()
	maskInfo.y = (row * 240) + maskInfo.y
	maskInfo.width = file.get_32()
	maskInfo.height = file.get_32()
	maskInfo.size = size - 16
	
	var key = maskInfo.x * 1000 + maskInfo.y
	if not self.no_repeat.has(key):
		var id = self.no_repeat.size()
		
		self.no_repeat[key] = id
		self.blocks[blockIndex].ownMasks.append(id)
		self.masks[id] = maskInfo
	else:
		var id = self.no_repeat[key] 
		self.blocks[blockIndex].ownMasks.append(id)
	
func readCell(file: File, offset: int, blockIndex: int, size: int):
	var row = blockIndex / self.colNum
	var col = blockIndex % self.colNum
	file.seek(offset)
	var cell = file.get_buffer(size)
	var i = 0
	var j = 0
	for c in cell:
		var ii = row * 12 + i
		var jj = col * 16 + j
		self.astar.add_point((ii * 1000 + jj), Vector2(ii, jj), c)
		j ++ 1
		if j >= 16:
			j = 0
			i += 1
	
func getMap(i):
	if i >= blockOffset.size() : return
	
	var file = File.new()
	var img
	file.open(path, File.READ)
	
	var offset =  self.blocks[i].jpegOffset
	var size = self.blocks[i].jpegSize
	
	file.seek(offset)
	var jpeg = file.get_buffer(size)
	if self.flag == "0.1M":
		var pba = gdxy.repair_jpeg(jpeg)
		img = Image.new()
		var err = img.load_jpg_from_buffer(pba)

	file.close()
	return img

func getMask(i):
	if i >= self.masks.size() : return
	
	var offset =  self.masks[i].offset
	var size = self.masks[i].size
	
	var file = File.new()
	file.open(path, File.READ)
	file.seek(offset)

	var img = Image.new()
	var maskBuffer = file.get_buffer(size)
	var pba = gdxy.read_mask(maskBuffer, masks[i].width, masks[i].height)
	img.create_from_data(masks[i].width, masks[i].height, false, Image.FORMAT_RGBA8, pba)

	file.close()
	return img
