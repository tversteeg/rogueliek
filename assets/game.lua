state = "Menu"

function keydown(keycode)
	print(keycode)
end

function setup()
	print("Setup")
end

function update()
	if state == "Menu" then
		drawstring(1, 0, "Hello world!", 255, 255, 255)
	else
	end
end
