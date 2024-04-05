from django.shortcuts import render
import json

from django.http import HttpResponse

def merchant_dashboard(request):
    return render(request, 'payment/merchant_dashboard.html')

def transactions(request):
    return HttpResponse(json.dumps({'transactions': []}), content_type='application/json')