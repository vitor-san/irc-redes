import tkinter as tk
from tkinter import font as tkfont
import os

if os.environ.get('DISPLAY', '') == '':
    print('no display found. Using :0.0')
    os.environ.__setitem__('DISPLAY', ':0.0')

colors = {
    "main": "#ff9000",
    "sec": "#fafa0f",
    "black": "#34252f",
    "white": "#f8e5ee",
}

# # the container is where we'll stack a bunch of frames
# # on top of each other, then the one we want visible
# # will be raised above the others
# main_frame = tk.Frame(main)
# # occupies all the space
# container.pack(side="top", fill="both", expand=True)
# container.grid_rowconfigure(0, weight=1)
# container.grid_columnconfigure(0, weight=1)

# Create main window
main = tk.Tk()
main.title("Cliente IRC - GG Club")

main.font = tkfont.Font(family='Ubuntu Mono', size=14)
main.geometry("600x500")
main['bg'] = colors['main']

# # Create the frames
# out_frame = tk.Frame(
#     main, bg=colors['black'], width=385, height=460, relief='raised', borderwidth=5)
# in_frame = tk.Frame(
#     main, bg=colors['black'], width=385, height=460, relief='raised', borderwidth=5)
# out_frame.pack(side="top", fill="x")
# in_frame.pack(side="top", fill="x")

# for frame in [out_frame, in_frame]:
#     frame.pack(expand=True, fill='both')
#     frame.pack_propagate(0)

termf = tk.Frame(main, height=200, width=400)
termf.pack(fill='both', expand=True)
wid = termf.winfo_id()
print(wid)
os.system(f"xterm -into {wid} -geometry 300x200 -e /root/.bashrc&")

# Start running!
main.mainloop()
