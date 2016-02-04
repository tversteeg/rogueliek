local rogue = {}

function rogue.init()
	print("Rogue!")

	floordir = assetdir .. "rogue/tiles/dc-dngn/floor/"
	loadpng(floordir .. "sandstone_floor0.png", "sandstone_floor0")
end

function rogue.update()
	clear()
end

function rogue.render()

end

return rogue
