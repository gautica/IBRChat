# IBRChat
Protocol for client-server communication:
ch[nick_name]				// client handshake
cco[name]:[channel]:[message]		// client chatting mode for one to one
ccm[channel]:[message]			// client chatting mode for one to many
cb[cmd]					// client Befehle mode
(Symbol ":" darf nicht in client_name und Kanal_name auftauchen
Protocol for Command:
