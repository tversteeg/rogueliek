local diablo = {}

function diablo.init()
	print("Diablo!")
	loadpng(assetdir .. "diablo/zombie.png", "zombie")
end

function diablo.update()
	clear()
end

return diablo
