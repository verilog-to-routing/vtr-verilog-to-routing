chmod -R 777 benchtracker
rm -rf benchtracker
git clone https://github.com/LemonPi/benchtracker.git
rm -rf benchtracker/.git
rm -rf benchtracker/.gitignore
sed -i "s/app.run(debug=True, port=port)/app.run(host='0.0.0.0', port=port)/" benchtracker/server_db.py
