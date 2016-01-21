state = "Menu"
menu_items = {"New Game", "Settings"}
menu_selected = 1

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
			clear()
		end
	end

	print(keycode)
end

function setup()
	print("Setup")
end

function update()
	if state == "Menu" then
		drawstring(getwidth() / 2 - string.len("Rogueliek!") / 2, 0, "Rogueliek!", 255, 255, 255)
		for i = 1, 2 do
			local color = {127, 127, 127}
			if menu_selected == i then
				color = {255, 255, 255}
			end
			drawstring(getwidth() / 2 - string.len(menu_items[i]) / 2, 2 + i * 2, menu_items[i], color[1], color[2], color[3])
		end
	else
	end
end
