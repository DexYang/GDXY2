extends Object

var GDXY2 = preload("res://bin/gdexample.gdns")
var gdxy2 = GDXY2.new()

const BLOCK_WIDTH = 320.0 #地图块宽度
const BLOCK_HEIGHT = 240.0 #地图块高度

var path         #文件路径
var header       #文件头
var maskIndexs   #遮罩头文件
var blockOffset  #地图块偏移信息

var image        #图片

func _init(path):
	if path == null: return
	self.path = path
	#读取文件头
	header = {}
	var file = File.new()
	file.open(path, File.READ)
	header.flag = file.get_buffer(4).get_string_from_utf8()
	header.width = file.get_32()
	header.height = file.get_32()
	header.rowNum = ceil(header.height / BLOCK_HEIGHT)
	header.colNum = ceil(header.width / BLOCK_WIDTH)
	print(header)
	if header.flag == "0.1M":
		newMapRead(file)
	file.flush()
	file.close()
	
func getImage(id):
	if id >= blockOffset.size() : return
	var file = File.new()
	var img
	file.open(path, File.READ)
	file.seek(blockOffset[id])
	var blockIdx = file.get_32()
	if blockIdx != 0:
		file.get_buffer(blockIdx*4)
	var headMark = file.get_buffer(4).get_string_from_utf8()
	if headMark == "GEPJ":
		var imgSize = file.get_32()
		var imgBuffer = file.get_buffer(imgSize)
		
		var pba = gdxy2.read_jpeg(imgBuffer)
		print(pba.size())
		img = Image.new()
		img.create_from_data(320, 240, false, Image.FORMAT_RGB8, pba)


	headMark = file.get_buffer(4).get_string_from_utf8()
	if headMark == "LLEC": #路径点
		var waySize = file.get_32()
		var wayBuffer = file.get_buffer(waySize)
		
	file.flush()
	file.close()
	return img

func getMask(id):
	if id >= maskIndexs.size() : return
	var file = File.new()
	file.open(path, File.READ)
	file.seek(maskIndexs[id])
	var maskHeader = {}
	maskHeader.x = file.get_32()
	maskHeader.y = file.get_32()
	maskHeader.width = file.get_32()
	maskHeader.height = file.get_32()
	maskHeader.size = file.get_32()
	var img = Image.new()
	if maskHeader.width > 0 && maskHeader.height > 0 && maskHeader.size > 0:
		#读取遮罩信息
		var maskBuffer = file.get_buffer(maskHeader.size)
		var pba = gdxy2.read_mask(maskBuffer, maskHeader.width, maskHeader.height)
		img.create_from_data(maskHeader.width, maskHeader.height, false, Image.FORMAT_RGBA8, pba)

	
	file.flush()
	file.close()
	return img

func newMapRead(file):
	blockOffset = []
	blockOffset.resize(header.rowNum * header.colNum)
	blockOffset.fill(0)
	for i in range(blockOffset.size()):
		blockOffset[i] = file.get_32()
	#print(blockOffset)
	file.get_32() #无用的4字节
	var maskNum = file.get_32()
	#print(maskNum)
	maskIndexs = []
	maskIndexs.resize(maskNum)
	maskIndexs.fill(0)
	for i in range(maskIndexs.size()):
		maskIndexs[i] = file.get_32()
#	
