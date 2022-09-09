extends Node2D

const MAP = preload("map.gd") # Relative path
onready var map = MAP.new("D:/Godot/gdxy2/demo/1410.map")



var image
var should_update_canvas = false
var drawing = false

# Called when the node enters the scene tree for the first time.
func _ready():
	create_image()
	update_texture()
	$Sprite.offset = Vector2(image.get_width() / 2, image.get_height() / 2)
	
func create_image():
	image = map.getImage(0)
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
