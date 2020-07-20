import pexpect
import threading
import time


# def send(proc):
#     for i in range(1, 21):
#         proc.sendline(f"mensagem {i} do cliente")
#         time.sleep(3)


# def receive(proc):
#     while True:
#         proc.expect('\n')
#         print(proc.before)
#         time.sleep(1)


class Bridge:
    def __init__(self):
        # Inicializo meu programa
        self.proc = pexpect.spawn('./client', encoding='utf-8')

        # threads = []

        # # Create new threads
        # in_thread = threading.Thread(target=lambda: send(proc))
        # out_thread = threading.Thread(target=lambda: receive(proc))

        # # Start new Threads
        # in_thread.start()
        # out_thread.start()

        # # Add threads to thread list
        # threads.append(in_thread)
        # threads.append(out_thread)

        # # Wait for all threads to complete
        # for t in threads:
        #     t.join()
        # print("Exiting program...")

    def sendline(self, message):
        if len(message) != 0:
            self.proc.sendline(message)

    def getline(self):
        self.proc.expect('\n')
        # return everything before the newline
        return self.proc.before

    def interact(self):
        self.proc.interact()


if __name__ == '__main__':
    b = Bridge()
    b.sendline('salve1')
    print(b.getline())
    b.sendline('salve2')
    print(b.getline())
    b.sendline('salve3')
    print(b.getline())
    b.interact()
