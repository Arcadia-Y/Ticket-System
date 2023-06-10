from flask import Flask, render_template, redirect, request
import sjtu

app = Flask(__name__)
tsys = sjtu.Sys()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/add_user', methods=["GET", "POST"])
def add_user():
    if request.method == "GET":
        return render_template("add_user.html")
    c = request.form.get("c")
    u = request.form.get("u")
    p = request.form.get("p")
    n = request.form.get("n")
    m = request.form.get("m")
    g = request.form.get("g")
    if g == "":
        g = "10"
    if tsys.add_user(c, u, p, n, m, int(g)) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/login', methods=["GET", "POST"])
def login():
    if request.method == "GET":
        return render_template("login.html")
    u = request.form.get("username")
    p = request.form.get("password")
    if tsys.login(u, p) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/logout', methods=["GET", "POST"])
def logout():
    if request.method == "GET":
        return render_template("logout.html")
    u = request.form.get("username")
    if tsys.logout(u) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/query_profile', methods=["GET", "POST"])
def query_profile():
    if request.method == "GET":
        return render_template("query_profile.html")
    c = request.form.get("c")
    u = request.form.get("u")
    res = tsys.query_profile(c, u)
    if len(res) == 0:
        return render_template("failure.html")
    return render_template("show_profile.html", records=res)

@app.route('/modify_profile', methods=["GET", "POST"])
def modify_profile():
    if request.method == "GET":
        return render_template("modify_profile.html")
    c = request.form.get("c")
    u = request.form.get("u")
    p = request.form.get("p")
    n = request.form.get("n")
    m = request.form.get("m")
    g = request.form.get("g")
    res = tsys.modify_profile(c, u, p, n, m, g)
    if len(res) == 0:
        return render_template("failure.html")
    return render_template("show_profile.html", records=res)

@app.route('/query_train', methods=["GET", "POST"])
def query_train():
    if request.method == "GET":
        return render_template("query_train.html")
    i = request.form.get("i")
    d = request.form.get("d")
    res = tsys.query_train(i, d)
    if len(res) == 0:
        return render_template("failure.html")
    tmp = res[1:]
    return render_template("show_train.html", train_id=res[0][0], train_type=res[0][1], records=res[1:])

@app.route('/add_train', methods=["GET", "POST"])
def add_train():
    if request.method == "GET":
        return render_template("add_train.html")
    i = request.form.get("i")
    n = request.form.get("n")
    m = request.form.get("m")
    s = request.form.get("s")
    p = request.form.get("p")
    x = request.form.get("x")
    t = request.form.get("t")
    o = request.form.get("o")
    d = request.form.get("d")
    y = request.form.get("y")
    if tsys.add_train(i, int(n), int(m), s, p, x, t, o, d, y) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/release_train', methods=["GET", "POST"])
def release_train():
    if request.method == "GET":
        return render_template("release_train.html")
    i = request.form.get("i")
    if tsys.release_train(i) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/delete_train', methods=["GET", "POST"])
def delete_train():
    if request.method == "GET":
        return render_template("delete_train.html")
    i = request.form.get("i")
    if tsys.delete_train(i) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/query_ticket', methods=["GET", "POST"])
def query_ticket():
    if request.method == "GET":
        return render_template("query_ticket.html")
    s = request.form.get("s")
    t = request.form.get("t")
    d = request.form.get("d")
    p = request.form.get("p")
    res = tsys.query_ticket(s, t, d, int(p))
    return render_template("show_ticket.html", cnt=len(res), records=res)

@app.route('/query_transfer', methods=["GET", "POST"])
def query_transfer():
    if request.method == "GET":
        return render_template("query_transfer.html")
    s = request.form.get("s")
    t = request.form.get("t")
    d = request.form.get("d")
    p = request.form.get("p")
    res = tsys.query_transfer(s, t, d, int(p))
    return render_template("show_transfer.html", cnt=len(res), records=res)
    

@app.route('/buy_ticket', methods=["GET", "POST"])
def buy_ticket():
    if request.method == "GET":
        return render_template('buy_ticket.html')
    u = request.form.get("u")
    i = request.form.get("i")
    d = request.form.get("d")
    n = request.form.get("n")
    f = request.form.get("f")
    t = request.form.get("t")
    q = request.form.get("q")
    res = tsys.buy_ticket(u, i, d, f, t, int(n), int(q))
    if res == -1:
        return render_template("success.html")
    elif res == -2:
        return render_template("queue.html")
    return render_template("buy_success.html", cost=res)

@app.route('/query_order', methods=["GET", "POST"])
def query_order():
    if request.method == "GET":
        return render_template("query_order.html")
    u = request.form.get("u")
    res = tsys.query_order(u)
    if len(res) == 0:
        return render_template("failure.html")
    return render_template("show_order.html", cnt=int(res[0][0]), records=res[1:])

@app.route('/refund_ticket', methods=["GET", "POST"])
def refund_ticket():
    if request.method == "GET":
        return render_template("refund_ticket.html")
    u = request.form.get("u")
    n = request.form.get("n")
    if tsys.refund_ticket(u, int(n)-1) == 0:
        return render_template("success.html")
    return render_template("failure.html")

@app.route('/clean')
def clean():
    tsys.clean()
    return render_template("success.html")

@app.errorhandler(404)
def page_not_found(e):
    return render_template('error.html'), 404
