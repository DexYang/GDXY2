extends Node2D

var MAP = load("map.gd") # Relative path
onready var map = MAP.new("D:/xy2/大话西游2/scene/1001.map")

var WDF = load("wdf.gd")
onready var wdf = WDF.new("D:/xy2/大话西游2/shape.wdf")

var image
var should_update_canvas = false
var drawing = false

# Called when the node enters the scene tree for the first time.
func _ready():
	create_image()
	update_texture()
	$Sprite.offset = Vector2(image.get_width() / 2, image.get_height() / 2)
	
#	var was = wdf.get("char\\0019\\defend.tcp")
#	var texture = ImageTexture.new()
#	texture.create_from_image(was.getFrame(0, 0).img)
#	$Sprite.set_texture(texture)
#	$Sprite.offset = Vector2(100, 100)
	
	
func create_image():
	image = map.getMap(0)
	# image.create(320, 240, false, Image.FORMAT_RGBA8)	
	# image.fill(Color.red)
	
func update_texture():
	var texture = ImageTexture.new()
	texture.create_from_image(image)
	$Sprite.set_texture(texture)
	should_update_canvas = false

func _process(delta):	
	if should_update_canvas:
		update_texture()
