import pyb
import songs
import uasyncio as asyncio
from rtttl import RTTTL

class UART_receiver():
    def __init__(self) -> None:
        self.uart = pyb.UART(1, 9600)
        self.data = ''

        self.uart.init(9600, bits=8, parity=None, stop=1)

        asyncio.create_task(self.run())

    async def run(self):
        while True:
            if self.uart.any():
                res = self.uart.read()
                self.data = res.decode("utf-8").strip()
            await asyncio.sleep(0)

    def getData(self):
        return self.data

class Radio():
    def __init__(self, uart) -> None:
        self.buz_tim = pyb.Timer(8, freq=440)
        self.buz_ch = self.buz_tim.channel(2, pyb.Timer.PWM, pin=pyb.Pin('Y2'), pulse_width=0)
        self.pwm = 50 # reduce this to reduce the volume
        self.uart = uart

    async def play_tone(self, freq, msec):
        print('freq = {:6.1f} msec = {:6.1f}'.format(freq, msec))
        if freq > 0:
            self.buz_tim.freq(freq)
            self.buz_ch.pulse_width_percent(self.pwm)
        pyb.delay(int(msec * 0.9))
        self.buz_ch.pulse_width_percent(0)
        pyb.delay(int(msec * 0.1))
        await asyncio.sleep(0)

    async def play(self, tune):
        try:
            for freq, msec in tune.notes():
                await self.play_tone(freq, msec)
                await asyncio.sleep(0)
                if self.uart.getData() == 's':
                    self.play_tone(0, 0)
                    break
                
        except KeyboardInterrupt:
            await self.play_tone(0, 0)

async def main():
    uart_msg = UART_receiver()
    radio = Radio(uart_msg)
    trackNumber = 7 # Just a random number to start somewhere
    rx_data = ''
    while True:
        await asyncio.sleep(0)
        rx_data = uart_msg.getData()

        if rx_data == 'p':
            track = RTTTL(songs.index(trackNumber))
            await radio.play(track)
        
        trackNumber = trackNumber + 1
        if trackNumber > songs.size():
            trackNumber = 0

asyncio.run(main())
