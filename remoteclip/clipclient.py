import time
import requests
import pyperclip
import binascii
import sys
import os
from Xlib import X, display, Xutil, Xatom
from Xlib.protocol import event
import json


# This script does not implement fancy stuff like INCR or MULTIPLE, so
# put a hard limit on the amount of data we allow sending this way
MAX_SIZE = 50000

def log(msg, *args):
    sys.stderr.write(msg.format(*args) + '\n')

def error(msg, *args):
    log(msg, *args)
    sys.exit(1)

d = display.Display()
w = d.screen().root.create_window(
        0, 0, 10, 10, 0, X.CopyFromParent)

# unavailable
def set_primary(data):
    global d

    sel_name = "PRIMARY"
    sel_atom = d.get_atom(sel_name)

    # map type atom -> data
    types = {}
    type_atom = d.get_atom("UTF8_STRING") 
    # type_atom = Xatom.STRING
    types[type_atom] = data.encode('UTF-8')

    targets_atom = d.get_atom('TARGETS')

    data_atom = d.get_atom('SEL_DATA')

    # We must have a window to own a selection
    global w
    
    # # And to grab the selection we must have a timestamp, get one with
    # # a property notify when we're anyway setting wm_name
    # w.change_attributes(event_mask = X.PropertyChangeMask)
    # w.set_wm_name(os.path.basename(sys.argv[0]))

    # e = d.next_event()
    # sel_time = e.time
    # w.change_attributes(event_mask = 0)

    # # Grab the selection and make sure we actually got it
    # w.set_selection_owner(sel_atom, sel_time)

    w.set_selection_owner(sel_atom, X.CurrentTime)
    if d.get_selection_owner(sel_atom) != w:
        log('could not take ownership of {0}', sel_name)
        return

    log('took ownership of selection {0}', sel_name)

    w.change_property( data_atom, type_atom, 8, types[type_atom])
   


def output_data(d, r, target_name):

    if r.format == 8:
        if r.property_type == Xatom.STRING:
            value = r.value.decode('ISO-8859-1')
        elif r.property_type == d.get_atom('UTF8_STRING'):
            value = r.value.decode('UTF-8')
        else:
            value = binascii.hexlify(r.value).decode('ascii')
        return value
    elif r.format == 32 and r.property_type == Xatom.ATOM:
        values = []
        for v in r.value:
            values.append(d.get_atom_name(v))
        return "\n".join(values)

    else:
        values = []
        for v in r.value:
            values.append(v)
        return "\n".join(values)

def get_primary():
    global d

    sel_name = "PRIMARY"
    sel_atom = d.get_atom(sel_name)

    target_name = 'UTF8_STRING'
    target_atom = d.get_atom(target_name)

    # Ask the server who owns this selection, if any
    owner = d.get_selection_owner(sel_atom)

    if owner == X.NONE:
        return None

    # Create ourselves a window and a property for the returned data
    global w
    w.set_wm_name(os.path.basename(sys.argv[0]))

    data_atom = d.get_atom('SEL_DATA')

    # The data_atom should not be set according to ICCCM, and since
    # this is a new window that is already the case here.

    # Ask for the selection.  We shouldn't use X.CurrentTime, but
    # since we don't have an event here we have to.
    w.convert_selection(sel_atom, target_atom, data_atom, X.CurrentTime)

    # Wait for the notification that we got the selection
    while True:
        e = d.next_event()
        if e.type == X.SelectionNotify:
            break
        # set_primary 需要在这里消费
        if (e.type == X.SelectionRequest
            and e.owner == w
            and e.selection == sel_atom):
            client_prop = e.property
            ev = event.SelectionNotify(
                time = e.time,
                requestor = e.requestor,
                selection = e.selection,
                target = e.target,
                property = client_prop)
            e.requestor.send_event(ev)

    # Do some sanity checks
    if (e.requestor != w
        or e.selection != sel_atom
        or e.target != target_atom):
        w.delete_property(data_atom)
        return None

    if e.property == X.NONE:
        w.delete_property(data_atom)
        return None

    if e.property != data_atom:
        w.delete_property(data_atom)
        return None

    # Get the data
    r = w.get_full_property(data_atom, X.AnyPropertyType,
                            sizehint = 10000)
    
    if not r:
        return

    # Can the data be used directly or read incrementally
    if r.property_type == d.get_atom('INCR'):
        w.delete_property(data_atom)
        return None
    else:
        w.delete_property(data_atom)
        return output_data(d, r, target_name)


def rreplace(s, old, new, occurrence):
    li = s.rsplit(old, occurrence)
    return new.join(li)


host = "10.52.1.80"
port = 8667
puturl = f"http://{host}:{port}/put"
geturl = f"http://{host}:{port}/get"
lastclipdata = ""
lastcliptime = 0
locallastclipdata = ""
while True:
    clipdata = get_primary()
    if clipdata and clipdata != locallastclipdata:
         print(clipdata)
         locallastclipdata = clipdata
         lastclipdata = clipdata
         lastcliptime = time.time()
         requests.post(url=puturl, headers={"Content-Type":"application/json"}, data=json.dumps({"content":clipdata, "time":lastcliptime}))
    remotedatadict = requests.post(url=geturl)
    remotedatadict = remotedatadict.json()
    remotedata = remotedatadict["content"]
    remotetime = remotedatadict["time"]
    if remotetime > lastcliptime:
        lastclipdata = remotedata
        lastcliptime = remotetime
        pyperclip.copy(remotedata)
        set_primary(remotedata)
    time.sleep(1)
