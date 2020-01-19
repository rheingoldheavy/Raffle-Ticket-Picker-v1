# Raffle-Ticket-Picker-v1
	
	Raffle Ticket Selection Code 
	
	Designed for use at Los Angeles Hardware Happy Hour (LA3H)

	This code takes a CLOCK button press and retrieves a random value within the bounds of the upper and
	lower raffle ticket numbers specified in the code. The raffle ticket values selected are tracked in the
	"previousWinners" array so duplicates aren't issued.  If all available values are selected, code operation
	is moved into a while(1) loop to prevent re-execution of the selection code, and just displays "FULL" on
	subsequent button presses.

	While waiting for user input, the sparkleDisplay function is executed.

	Random Seed Generator Documentation:    https://rheingoldheavy.com/better-arduino-random-values/
	Display = I2C Display Add On:           https://rheingoldheavy.com/product/i2c-display-add/
	Shield  = I2C and SPI Education Shield: https://rheingoldheavy.com/product/i2c-and-spi-education-shield/
	ISSI 31FL3728 Datasheet:                http://www.issi.com/WW/pdf/31FL3728.pdf
