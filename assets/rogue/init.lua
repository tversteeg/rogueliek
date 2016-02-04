local rogue = {}

map = require(assetdir .. "rogue/map")

function rogue.init()
	print("Rogue!")

	floordir = assetdir .. "rogue/tiles/dc-dngn/floor/"
	loadpng(floordir .. "sandstone_floor0.png", "sandstone_floor0")

	map.create(100, 100, 20)
end

function rogue.update()
end

function rogue.render()
	clear()
	map.render()
end

return rogue
