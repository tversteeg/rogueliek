local diablo = {}

function diablo.init()
	print("Diablo!")
	loadpng(assetdir .. "diablo/zombie.png", "zombie")
end

function diablo.update()
	drawpngname("zombie", 10, 10)
end

return diablo
