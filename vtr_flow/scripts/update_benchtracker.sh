chmod -R 777 benchtracker
rm -rf benchtracker
git clone https://github.com/LemonPi/benchtracker.git
rm -rf benchtracker/.git
rm -rf benchtracker/.gitignore
sed -i "s/app.run(debug=True)/app.run(host='0.0.0.0')/" benchtracker/server_db.py
chmod -R ugo-w benchtracker
