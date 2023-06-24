from websocket import create_connection

#ws = create_connection("ws://echo.websocket.events/")
ws = create_connection("ws://localhost:9001")
print(ws.recv())
print("Sending 'Hello, World'...")
ws.send("Hello, World")
print("Sent")
print("Receiving...")
result =  ws.recv()
print("Received '%s'" % result)
ws.close()