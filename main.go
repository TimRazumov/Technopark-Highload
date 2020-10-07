package main

import (
	"github.com/labstack/echo/v4"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"log"
	"math/rand"
	"net/http"
	"time"
)

func main() {
	hitsTotal := prometheus.NewCounter(prometheus.CounterOpts{
		Name: "hits_total",
	})
	if err := prometheus.Register(hitsTotal); err != nil {
		log.Fatal(err)
	}
	if err := prometheus.Register(prometheus.NewBuildInfoCollector()); err != nil {
		log.Fatal(err)
	}
	go func() {
		metricsRouter := echo.New()
		metricsRouter.GET("/metrics", echo.WrapHandler(promhttp.Handler()))
		log.Fatal(metricsRouter.Start(":5050"))
	}()

	router := echo.New()
	router.GET("/api",
		func(ctx echo.Context) error {
			defer hitsTotal.Inc()
			i := time.Duration(rand.Intn(5))
			time.Sleep(i * time.Second)
			return ctx.String(http.StatusOK, "hello")
		},
	)
	log.Fatal(router.Start(":8080"))
}
