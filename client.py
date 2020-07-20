import tkinter as tk
from tkinter import font as tkfont
import os
from bridge import Bridge
import threading

if os.environ.get('DISPLAY', '') == '':
    print('no display found. Using :0.0')
    os.environ.__setitem__('DISPLAY', ':0.0')

colors = {
    "main": "#ff9000",
    "sec": "#c1f80a",
    "black": "#34252f",
    "white": "#f8e5ee",
}

# Connect to cpp program
cli = Bridge()

# Create main window
main = tk.Tk()
main.title("Cliente IRC - GG Club")

font = tkfont.Font(family='System', size=10, weight='bold')
main.font = font
main.geometry("900x500")
main['bg'] = colors['main']

# Create the frames
out_frame = tk.Frame(
    main, bg=colors['black'], height=420, relief='raised', borderwidth=5)
in_frame = tk.Frame(
    main, bg=colors['black'], height=60, relief='raised', borderwidth=5)

# Rendering
pad = 10
out_frame.pack(side="top", fill="x", padx=pad, pady=(pad, pad/2))
in_frame.pack(side="top", fill="x", padx=pad, pady=(pad/2, pad))

# Don't let children widgets dictate my size
in_frame.pack_propagate(0)
out_frame.pack_propagate(0)

scrollbar = tk.Scrollbar(out_frame, bg=colors['black'])
scrollbar.pack(side='right', fill='y')

messages = tk.Text(
    out_frame, bg=colors['black'], fg=colors['sec'], padx=10, pady=10, spacing3=5, yscrollcommand=scrollbar.set, font=font)
messages.pack(fill='both', expand=True)
messages.config(state='disabled')
scrollbar.config(command=messages.yview)

user_input = tk.StringVar()
input_field = tk.Entry(in_frame, text=user_input, borderwidth=0, highlightthickness=0,
                       bg=colors['black'], fg=colors['sec'], font=font)
input_field.pack(fill='both', expand=True, padx=10, pady=10)


def receive():
    while True:
        try:
            line = cli.getline()
            messages.config(state='normal')
            messages.insert('insert', "{}".format(line.replace('\r', '')))
            messages.see(tk.END)
            messages.config(state='disabled')
        except:
            print("Connection closed with client program (EOF).")
            break


out_thread = threading.Thread(target=lambda: receive())
out_thread.start()


def enter_pressed(event):
    input_get = input_field.get()
    print(input_get)
    cli.sendline(input_get)
    user_input.set('')
    return "break"


input_field.bind("<Return>", enter_pressed)

# Start running!
main.mainloop()

out_thread.join()
cli.kill()
print('Exiting.')
