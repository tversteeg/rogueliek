local diablo = {}

function diablo.init()
	print("Diablo!")
	loadpng(assetdir .. "diablo/zombie.png", "zombie")
end

function diablo.update()
end

function diablo.render()
	clear()
	drawpngname("zombie", 10, 10)
	drawstring(2, 2, "Bla", 255, 0, 0)
end

return diablo
