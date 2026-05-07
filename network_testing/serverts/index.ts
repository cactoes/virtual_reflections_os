Bun.serve({
    hostname: "0.0.0.0",
    port: 80,
    routes: {
        "/": () => {
            console.log("new request");
            return new Response("net");
        }
    }
});