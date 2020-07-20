import pexpect


class Bridge:
    def __init__(self):
        self.proc = pexpect.spawn('./client', encoding='utf-8')

    def sendline(self, message):
        if len(message) != 0:
            self.proc.sendline(message)

    def getline(self):
        self.proc.expect('.+', timeout=None)
        # return everything
        return self.proc.after

    def kill(self):
        self.proc.sendcontrol('d')
