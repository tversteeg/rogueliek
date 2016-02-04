diablo = require(assetdir .. "diablo")
rogue = require(assetdir .. "rogue")

state = "Menu"
game = nil

menu_items = {"Roguelike", "Diablolike", "FFlike"}
menu_len = 3
menu_selected = 1

function renderselectionmenu(width, height)
	clear()
	for i = 1, menu_len do
		local color = {127, 127, 127}
		if menu_selected == i then
			color = {255, 255, 255}
		end
		drawstring(width / 2 - math.floor(string.len(menu_items[i]) / 2), 8 + i * 4, menu_items[i], color[1], color[2], color[3])
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

left_pressed = false
right_pressed = false
top_pressed = false
bot_pressed = false

function keydown(keycode)
	if state == "Menu" then
		if keycode == 65364 then
			if menu_selected < menu_len then
				menu_selected = menu_selected + 1
			end
		elseif keycode == 65362 then
			if menu_selected > 1 then
				menu_selected = menu_selected - 1
			end
		elseif keycode == 65293 then
			state = "Game"
			if menu_items[menu_selected] == "Roguelike" then
				game = rogue
			elseif menu_items[menu_selected] == "Diablolike" then
				game = diablo
			end
			game.init()
		end
	elseif state == "Game" then
		if keycode == 65364 then
			bot_pressed = true
		elseif keycode == 65362 then
			top_pressed = true
		elseif keycode == 65363 then
			right_pressed = true
		elseif keycode == 65361 then
			left_pressed = true
		end
	end
end

function keyup(keycode)
	if keycode == 65364 then
		bot_pressed = false
	elseif keycode == 65362 then
		top_pressed = false
	elseif keycode == 65363 then
		right_pressed = false
	elseif keycode == 65361 then
		left_pressed = false
	end
end

function setup()
	math.randomseed(os.time())
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

	if state == "Menu" then
		renderselectionmenu(width, height)
	elseif state == "Game" then
		game.update()
	end
end

function render()
	if state == "Game" then
		game.render()
	end
end
