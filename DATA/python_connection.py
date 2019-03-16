import sqlite3
conn = sqlite3.connect('./pills.db')

c = conn.cursor()

c.execute("headers ON")
c.execute('''.mode column''')
c.execute('''SELECT * FROM users_info''')

conn.commit()

conn.close()
