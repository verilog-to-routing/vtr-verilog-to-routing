chmod -R 777 benchtracker
mv benchtracker_moved
git clone https://github.com/LemonPi/benchtracker.git
rm -rf benchtracker/.git
rm -rf benchtracker/.gitignore
mv benchtracker_moved/.svn benchtracker
rm -rf bentracker_moved
sed -i "s/app.run(debug=True, port=port)/app.run(host='0.0.0.0', port=port)/" benchtracker/server_db.py
