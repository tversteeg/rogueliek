state = "Menu"
menu_items = {"New Game", "Settings"}
menu_selected = 1

map_size = 256

player = {
	x = 40, y = 40
}

function keydown(keycode)
	if state == "Menu" then
		if keycode == 65364 then
			if menu_selected < 2 then
				menu_selected = menu_selected + 1
			end
		elseif keycode == 65362 then
			if menu_selected > 1 then
				menu_selected = menu_selected - 1
			end
		elseif keycode == 65293 then
			state = menu_items[menu_selected]
		end
	elseif state == "Game" then
		if keycode == 65364 then
			player.y = player.y + 1
		elseif keycode == 65362 then
			player.y = player.y - 1
		elseif keycode == 65363 then
			player.x = player.x + 1
		elseif keycode == 65361 then
			player.x = player.x - 1
		end
	end

	print(keycode)
end

function setup()
	generatemap(getwidth(), getheight(), 0, 10, 50)
end

function update()
	local width = getwidth()
	if math.fmod(width, 2) == 1 then
		width = width - 1
	end
	local height = getheight()
	if math.fmod(height, 2) == 1 then
		height = height - 1
	end

	clear()
	if state == "New Game" then
		drawstring(width / 2 - 7, height / 2, "Generating map", 255, 255, 255)
		state = "Generate Map"
	elseif state == "Generate Map" then
		generatemap(map_size, map_size, 0, 10, 100)
		state = "Game"
	elseif state == "Game" then
		for i = 1, width - 15 do
			drawchar(i, 0, string.byte("="), 64, 64, 64)
			drawchar(i, height - 2, string.byte("="), 64, 64, 64)
		end
		for i = 0, height - 2 do
			drawchar(0, i, string.byte("#"), 64, 64, 64)
			drawchar(width - 15, i, string.byte("#"), 64, 64, 64)
		end
		rendermap(1, 1, width - 16, height - 3, player.x - width / 2, player.y - height / 2)
		drawchar((width - 14) / 2, (height - 2) / 2, string.byte("@"), 255, 255, 255)
	elseif state == "Menu" then
		rendermap(2, 2, width - 4, height - 3, 0, 0)
		drawstring(width / 2 - string.len("Grottwesens!") / 2, 3, "Grottwesens!", 255, 128, 128)
		for i = 1, 2 do
			local color = {127, 127, 127}
			if menu_selected == i then
				color = {255, 255, 255}
			end
			drawstring(width / 2 - string.len(menu_items[i]) / 2, 8 + i * 4, menu_items[i], color[1], color[2], color[3])
		end

		for i = 1, width - 2 do
			drawchar(i, 1, string.byte("="), 64, 64, 64)
			drawchar(i, height - 1, string.byte("="), 64, 64, 64)
		end
		for i = 1, height - 1 do
			drawchar(1, i, string.byte("#"), 64, 64, 64)
			drawchar(width - 2, i, string.byte("#"), 64, 64, 64)
		end
	end
end
